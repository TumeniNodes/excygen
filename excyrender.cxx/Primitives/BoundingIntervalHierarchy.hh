// (C) 2013 Sebastian Mach (1983), this file is published under the terms of the
// GNU General Public License, Version 3 (a.k.a. GPLv3).
// See COPYING in the root-folder of the excygen project folder.
#ifndef BOUNDINGINTERVALHIERARCHY_HH_INCLUDED_20130721
#define BOUNDINGINTERVALHIERARCHY_HH_INCLUDED_20130721

#include "Primitives/FinitePrimitive.hh"
#include "Shapes/FiniteShape.hh"
#include "Geometry/Direction.hh"
#include <cstdint>
#include <iterator>

namespace excyrender { namespace Primitives { namespace detail {

    struct bih_node {
        uint32_t flags : 2;
        uint32_t index : 30;
        float clip[2];

        static bih_node Inner(tuple<real,real> const &clip, int axis, int index)
        {
            if (axis!=0 && axis!=1 && axis!=2)
                throw std::logic_error("bih_node::Inner: axis must be one of 0,1,2");
            bih_node ret;
            ret.clip[0] = get<0>(clip);
            ret.clip[1] = get<1>(clip);
            ret.flags = axis;
            ret.index = index;
            return ret;
        }

        static bih_node Leaf(int index)
        {
            bih_node ret;
            ret.flags = 3;
            ret.index = index;
            return ret;
        }

        static bih_node Bogus()
        {
            return bih_node();
        }

        bool empty() const { return std::fabs(clip[0]-clip[1])<=0; }
        bool leaf () const { return flags == 3; }

    private:
        bih_node() = default;
    };

    inline
    std::ostream& operator<< (std::ostream& os, bih_node const &n) {
        if (n.leaf()) {
            os << "leaf{" << n.index << "}";
        } else {
            os << "innr{[" << n.clip[0] << ".." << n.clip[1] << "], " << n.flags << "}";
        }
        return os;
    }

    template <typename T>
    struct bih_data {
        std::vector<T> objects;

        AABB aabb = AABB(Geometry::Point(0,0,0),Geometry::Point(1,1,1)); // we could skip this meaningless initialization by separating builder, data and traverser
        std::vector<bih_node> nodes;

        typedef typename std::vector<T>::const_iterator object_iterator;
        typedef tuple<object_iterator, object_iterator> object_group;
        std::vector<object_group> object_groups;
    };




    template <typename T>
    class bih_builder
    {
        typedef typename std::vector<T>::iterator iterator;
    public:
        void start_build(bih_data<T> &data, int recursions_left = 10)
        {
            data.aabb = exact_aabb(data.objects.begin(), data.objects.end());

            build_node(data.objects.begin(), data.objects.end(),
                       data.aabb,
                       0,
                       data.nodes,
                       data.object_groups);
            data.nodes.shrink_to_fit();
        }

    private:

        static AABB exact_aabb(iterator it, iterator end) {
            Geometry::Point Max(-real_max, -real_max, -real_max),
                            Min(real_max, real_max, real_max);
            for ( ; it!=end; ++it) {
                const auto bb = it->aabb();
                Max.x = max(Max.x, right(bb));
                Max.y = max(Max.y, top(bb));
                Max.z = max(Max.z, back(bb));
                Min.x = min(Min.x, left(bb));
                Min.y = min(Min.y, bottom(bb));
                Min.z = min(Min.z, front(bb));
            }
            return {Min, Max};
        }

        static tuple<real,real> clip(iterator it, iterator end, int axis) {
            real Max = -real_max, Min = real_max;
            for ( ; it!=end; ++it) {
                const auto bb = it->aabb();
                Max = max(Max, bb.max()[axis]);
                Min = min(Min, bb.min()[axis]);
            }
            return make_tuple(Min, Max);
        }

        static void build_node(const iterator first, const iterator last, AABB const &node_bb,
                               int r,
                               std::vector<bih_node> &nodes,
                               std::vector<typename bih_data<T>::object_group> &groups)
        {
            using namespace Geometry;

            const int  axis = longest_axis(node_bb);
            const auto clip_planes = clip(first, last, axis);

            if (std::distance(first, last) <= 1 || r>5)
            {
                groups.emplace_back(first, last);
                nodes.push_back(bih_node::Leaf(groups.size()-1));
                return;
            }
            else
            {
                // Reserve space.
                nodes.push_back (bih_node::Bogus());
                const auto where_our_node_at = nodes.size()-1;

                // Find pivot object.
                const real split_plane = center(node_bb, axis);
                const auto pivot = std::partition (first, last, [&](T const &obj) {
                                                 return center(obj.aabb())[axis] < split_plane; });

                // Children bounding boxes, children, and current node finalization.
                const auto children_bb = split(node_bb, axis);
                build_node(first, pivot, get<0>(children_bb), r+1, nodes, groups);
                nodes[where_our_node_at] = bih_node::Inner(clip_planes, axis,
                                                           nodes.size()-where_our_node_at);
                build_node(pivot, last,  get<1>(children_bb), r+1, nodes, groups);
            }
        }

        static void debug (std::ostream &os, bih_node const *node, int deep=0) {
            for (int i=0; i<deep*4; ++i)
                os << ' ';
            os << *node << '\n';
            if (node->leaf())
                return;
            debug(os, node+1, deep+1);
            debug(os, node+node->index, deep+1);
        }

    };



    template <typename T, typename BaseClass>
    class bih_traverser : public BaseClass {
    public:
        bih_traverser(bih_data<T> &data) : data(data) {}

        auto intersect(Geometry::Ray const &ray) const noexcept
        -> decltype(((T*)(nullptr))->intersect(ray))
        {
            using RetT = decltype(((T*)(nullptr))->intersect(ray));
            const auto initial = excyrender::intersect(data.aabb, ray);
            if (!initial)
                return RetT();
            const real A = max(real(0),get<0>(*initial)),
                       B = get<1>(*initial);
            return traverse_rec(&data.nodes[0], ray, A, B);
        }

        bool occludes(Geometry::Point const &a, Geometry::Point const &b) const noexcept
        {
            return !!intersect(Geometry::Ray(a, Geometry::Direction::Normalize(b-a)));
        }

        bool occludes(Geometry::Point const &a, Geometry::Direction const &b) const noexcept
        {
            return !!intersect(Geometry::Ray(a, b));
        }

    private:
        bih_data<T> &data;

    private:
        auto traverse_rec(bih_node const* node, Geometry::Ray const &ray, real A, real B) const
        -> decltype(((T*)(nullptr))->intersect(ray))
        {
            using RetT = decltype(((T*)(nullptr))->intersect(ray));

            if (A >= B) {
                return RetT();
            }

            if (node->leaf())
            {
                RetT nearest;
                typename bih_data<T>::object_group g = data.object_groups[node->index];
                for (auto it=get<0>(g), end=get<1>(g); it!=end; ++it) {
                    if (auto tmp = it->intersect(ray)) {
                        const auto t = distance(*tmp);
                        if (t<A || t>B)
                            continue;
                        if (!nearest || t < distance(*nearest)) {
                            nearest = tmp;
                        }
                    }
                }
                return nearest;
            }
            else
            {
                const int axis = node->flags;
                const real t1 = (node->clip[0] - ray.origin[axis]) / ray.direction[axis];
                const real t2 = (node->clip[1] - ray.origin[axis]) / ray.direction[axis];

                if (ray.direction[axis] < 0) {
                    auto a = traverse_rec(node+1, ray, A, min(t1,B));
                    if (a) B = min(B, distance(*a));
                    auto b = traverse_rec(node+node->index, ray, max(t2,A), B);

                    if (b) return b;
                    return a;
                } else {
                    auto a = traverse_rec(node+node->index, ray, A, min(t2,B));
                    if (a) B = min(B, distance(*a));
                    auto b = traverse_rec(node+1, ray, max(t1,A), B);

                    if (b) return b;
                    return a;
                }
            }
            return RetT();
        }
    };

} } }


namespace excyrender { namespace Primitives {

class BoundingIntervalHierarchy final : public FinitePrimitive {
public:
    BoundingIntervalHierarchy(BoundingIntervalHierarchy const &)            = delete;
    BoundingIntervalHierarchy& operator=(BoundingIntervalHierarchy const &) = delete;

    optional<Intersection> intersect(Geometry::Ray const &) const noexcept;
    bool occludes(Geometry::Point const &a, Geometry::Point const &b) const noexcept;
    bool occludes(Geometry::Point const &a, Geometry::Direction const &b) const noexcept;
    AABB aabb() const noexcept;

private:
    friend class BoundingIntervalHierarchyBuilder;

    BoundingIntervalHierarchy() = default;
    class Impl;
    std::shared_ptr<Impl> impl_;
};



class BoundingIntervalHierarchyBuilder {
public:
    class Group {
    public:
        Group()                         = delete;
        Group(Group const&)             = delete;
        Group& operator=(Group const &) = delete;

        void add (std::shared_ptr<Primitives::FinitePrimitive> shape) {
            if (*finalized) {
                throw std::logic_error("BoundingIntervalHierarchyBuilder::Group: called 'add()' "
                                       "but builder is finalized already");
            }
        }
    private:
        Group(BoundingIntervalHierarchyBuilder &builder)
            : finalized(builder.finalized)
        {
        }

        std::shared_ptr<bool> finalized;

        friend class BoundingIntervalHierarchyBuilder;
    };
    friend class Group;

    BoundingIntervalHierarchyBuilder() = default;
    BoundingIntervalHierarchyBuilder(BoundingIntervalHierarchyBuilder const &)            = delete;
    BoundingIntervalHierarchyBuilder& operator=(BoundingIntervalHierarchyBuilder const &) = delete;

    std::shared_ptr<Group> group(Photometry::Surface::BSDF bsdf) {
        if (*finalized) {
                throw std::logic_error("BoundingIntervalHierarchyBuilder::Group: called 'group()' "
                                       "but builder is finalized already");
        }
        return std::shared_ptr<Group>{new Group(*this)};
    }

    std::shared_ptr<BoundingIntervalHierarchy> finalize() {
        if (*finalized) {
                throw std::logic_error("BoundingIntervalHierarchyBuilder::Group: called 'finalize()' "
                                       "but builder is finalized already");
        }
        *finalized = true;
        return std::shared_ptr<BoundingIntervalHierarchy>{new BoundingIntervalHierarchy()};
    }

private:
    std::shared_ptr<bool> finalized = std::shared_ptr<bool>(new bool(false));
};


// TODO: once functional, add a specialized BIH for any type T <- FiniteShape

} }

#endif // BOUNDINGINTERVALHIERARCHY_HH_INCLUDED_20130721
