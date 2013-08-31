// (C) 2013 Sebastian Mach (1983), this file is published under the terms of the
// GNU General Public License, Version 3 (a.k.a. GPLv3).
// See COPYING in the root-folder of the excygen project folder.

#include "1100_resolve_types.hh"
#include "../ASTQueries/resolve_type.hh"

#include <stack>
#include <iostream>
#include <stdexcept>
#include <algorithm>



//- Tests ------------------------------------------------------------------------------------------
#include "../UnitTesting.hh"
#include "../ASTPrinters/PrettyPrinter.hh"

TEST_CASE( "Et1/ASTPasses/1100_resolve_types.hh", "Type resolution" ) {

    using namespace excyrender::Nature::Et1;
    using detail::equal;

    auto passes = [](std::shared_ptr<AST::Program> ast) { ASTPasses::resolve_types(ast); };

    /*REQUIRE(equal("let f(x) = y in f(2)",  // TODO: make this fail
                  "let int f(int x) = 1 in f(2)",
                  passes));*/

    REQUIRE(equal("let y = 1 in y",
                  "let int y = 1 in y",
                  passes));

    REQUIRE(equal("let auto y = 1 in y",
                  "let int y = 1 in y",
                  passes));

    REQUIRE(equal("let f(int x) = 1 in f(2)",
                  "let int f(int x) = 1 in f(2)",
                  passes));

    REQUIRE(equal("let f(int x) = x in f(2)",
                  "let int f(int x) = x in f(2)",
                  passes));

    REQUIRE(equal("let f(x) = x in f(2)",

                  "let f(x) = x, "
                  "    int f(int x) = x "
                  "in f(2)",
                  passes));

    REQUIRE(equal("let f(x) = x in f(2.0)",

                  "let f(x) = x, "
                  "    float f(float x) = x "
                  "in f(2.0)",
                  passes));


    REQUIRE(equal("let f(x) = 1 in f(2.0)",

                  "let f(x) = 1, "
                  "    int f(float x) = 1 "
                  "in f(2.0)",
                  passes));

    REQUIRE(equal("let f(x) = x, "
                  "    y = f(2.0) "
                  "in f(2)",

                  "let f(x) = x, "
                  "    float y = f(2.0), "
                  "    int f(int x) = x, "
                  "    float f(float x) = x "
                  "in f(2)",
                  passes));

return;

    REQUIRE(equal("let f(x) = 1.0 in f(2)",
                  "let float f(int x) = 1.0 in f(2)",
                  passes));

    REQUIRE(equal("let f(x) = 1.0 in f(2)",
                  "let float f(float x) = 1.0 in f(2.0)",
                  passes));

    // This tests auto-emitted overloads.
    REQUIRE(equal("let f(x) = 1, "
                  "    a = f(1), "
                  "    b = f(1.0) "
                  "in 1",

                  "let int f(int x) = 1, "
                  "    int f(float x) = 1, "
                  "    int a = f(1), "
                  "    int b = f(1.0) "
                  "in 1",
                  passes));

    REQUIRE(equal("let f(x,y) = 1, "
                  "    a = f(1, 1),  "
                  "    b = f(1.0, 1), "
                  "    c = f(1, 1.0), "
                  "    d = f(1.0, 1.0) "
                  "in 1",

                  "let int   f(int x, int y) = x, "
                  "    float f(float x, int y) = x, "
                  "    int   f(int x, float y) = x, "
                  "    float f(float x, float y) = x, "
                  "    int   a = f(1, 1),  "
                  "    float b = f(1.0, 1), "
                  "    int   c = f(1, 1.0), "
                  "    float d = f(1.0, 1.0) "
                  "in 1",
                  passes));

    // TODO: check for thrown exceptions upon type mismatches
}
//--------------------------------------------------------------------------------------------------



namespace excyrender { namespace Nature { namespace Et1 { namespace ASTPasses {

namespace {
    using namespace AST;
    using std::set;
    using std::stack;
    using std::string;

    struct ResolveTypes final : Transform {
        bool transformed() const { return transformed_; }
        bool has_unresolved() const { return has_unresolved_; }

        void begin(Addition &) {}
        void end(Addition &) {}

        void begin(Subtraction &) {}
        void end(Subtraction &) {}

        void begin(Multiplication &) {}
        void end(Multiplication &) {}

        void begin(Division &) {}
        void end(Division &) {}

        void begin(IntegerLiteral &) {}
        void end(IntegerLiteral &) {}

        void begin(RealLiteral &) {}
        void end(RealLiteral &) {}

        void begin(Call &) {}

        void end(Call &call)
        {
            // TODO: use end(Call&) when asts have their type resolved already
            std::vector<string> types;
            for (auto &arg : call.arguments()) {
                string type = arg->type();
                if (type.empty() || type[0] == '<' || type=="auto") {
                    has_unresolved_ = true;
                    return;
                }
                types.push_back(type);
            }

            auto fitness = [&](Binding &b) -> int {
                if (b.id() != call.id())
                    return -1;
                if (b.arguments().size() != call.arguments().size())
                    return -1;
                // count the number of matchin arguments.
                int f = 0;
                for (size_t i=0; i<b.arguments().size(); ++i) {
                    if (b.arguments()[i].type == "auto") {
                        f += 1;
                    } else if (b.arguments()[i].type == call.arguments()[i]->type()) {
                        f += 1+b.arguments().size();
                    } else {
                        return -1;
                    }
                }
                return f;
            };

            // lookup the binding to be called
            Binding* binding = nullptr;
            int best_fitness = 0;
            bool ambiguous = false;
            for (auto b : scope.top().visible_bindings) {
                int f = fitness(*b);

                if (f > best_fitness) {
                    binding = b; // TODO: build list of candidates
                    best_fitness = f;
                    ambiguous = false;
                } else if (f == best_fitness) {
                    ambiguous = true;
                }
            }

            /*
            std::cerr << "best fitness: " << best_fitness << std::endl;
            std::cerr << "id: " << binding->id() << std::endl;
            for (auto a : binding->arguments()) {
                std::cerr << a.type << std::endl;
            }
            std::cerr << std::endl;
            */

            if (ambiguous) {
                throw std::runtime_error("multiple instantiations of " + call.id() + " are ambiguous");
            }
            if (binding) {
                bool is_generic = false;
                for (auto arg : binding->arguments()) {
                    if (arg.type == "auto") {
                        is_generic = true;
                        break;
                    }
                }
                if (is_generic) {
                    if (scope.top().instantiate(*binding, types)) {
                        transformed_ = true;
                    }
                }
            }
        }

        void begin(Negation &) {}
        void end(Negation &) {}

        void begin(ParenExpression &) {}
        void end(ParenExpression &) {}

        void begin(Binding &binding)
        {
            //scope.top().visible_bindings.push_back(&binding);
            scope.push(scope.top().enter_binding(binding));
        }
        void end(Binding &binding)
        {
            // Resolve the return type only if the function is not generic.
            bool is_generic = false;
            for (auto arg : binding.arguments()) {
                if (arg.type == "auto") {
                    is_generic = true;
                    break;
                }
            }
            if (!is_generic) {
                auto type = ASTQueries::resolve_type(binding.body(), scope.top().symbols);
                if (!type.empty() && type[0] != '<') {
                    if (type != binding.type()) {
                        if (binding.type() != "auto") {
                            throw std::runtime_error(binding.id() + " declared " + binding.type() +
                                                     ", but function body is " + type);
                        }
                        binding.reset_type(type);
                        transformed_ = true;
                    }
                }
            }
            scope.pop();
        }

        void begin(AST::Identifier &) {}
        void end(AST::Identifier &) {}

        void begin(LetIn &letin) {
            scope.push(scope.top().enter_declarative_region(letin.bindings()));
        }
        void end(LetIn &) {
            scope.pop();
        }

        void begin(Program &p) {
            scope.push(Scope::EnterProgram(p.bindings()));
        }
        void end(Program &) {
            scope.pop();
        }

    private:
         bool transformed_ = false;
         bool has_unresolved_ = false;

         struct Scope {
             std::map<string, string> symbols;
             vector<shared_ptr<Binding>> *bindings_declarative_region;
             vector<Binding*> visible_bindings;

             Scope enter_binding(Binding &binding) const {
                Scope ret;
                ret.visible_bindings = visible_bindings;
                ret.visible_bindings.push_back(&binding);
                ret.bindings_declarative_region = bindings_declarative_region;

                for (auto a : binding.arguments()) {
                    ret.symbols[a.name] = a.type;
                }
                return ret;
             }

             Scope enter_declarative_region(vector<shared_ptr<Binding>> &bindings_declarative_region) const {
                Scope ret;
                ret = *this;
                for (auto b : bindings_declarative_region)
                    ret.visible_bindings.push_back(&*b);
                ret.bindings_declarative_region = &bindings_declarative_region;
                return ret;
             }

             static Scope EnterProgram (vector<shared_ptr<Binding>> &bindings_declarative_region) {
                Scope ret;
                for (auto b : bindings_declarative_region)
                    ret.visible_bindings.push_back(&*b);
                ret.bindings_declarative_region = &bindings_declarative_region;
                return ret;
             }

             // Returns true if instantiation happened.
             //         false if nothing happened.
             bool instantiate(Binding& binding, std::vector<string> types) {
                if (types.size() != binding.arguments().size())
                    throw std::runtime_error("wrong number of arguments in call to " + binding.id());

                // Check if this is the optimal candidate already.
                // If so, do not re-instantiate.
                bool is_optimal = true;
                for (size_t a=0, num_args=types.size(); a!=num_args; ++a) {
                    if (binding.arguments()[a].type != types[a]) {
                        is_optimal = false;
                        break;
                    }
                }
                if (is_optimal)
                    return false;

                shared_ptr<Binding> insta (binding.deep_copy());

                for (size_t a=0, num_args=insta->arguments().size(); a!=num_args; ++a) {
                    const auto &desired = types[a];
                    Argument &arg = insta->arguments()[a];

                    if (arg.type != "auto" && desired != arg.type)
                        throw std::runtime_error("cannot instantiate");
                    arg.type = desired;
                }

                bindings_declarative_region->push_back(insta);
                visible_bindings.push_back(&*insta);
                return true;
             }
         };

         std::stack<Scope> scope;
    };
}

void resolve_types(shared_ptr<AST::ASTNode> ast) {
    if (!ast) {
        //std::clog << "pass: lambda-lifting skipped, AST is empty\n";
        return;
    }

    int i = 0;
    while (1) {
        ++i;
        ResolveTypes ll;
        ast->accept(ll);
        if (!ll.transformed()) {
            if (ll.has_unresolved()) {
                throw std::runtime_error("program is not fully resolvable");
            }
            break;
        }

        ASTPrinters::PrettyPrinter pp(std::cerr);
        ast->accept(pp);
        std::cerr << "\n";
    }
    std::clog << "pass: resolve-types (" << i << "x)\n";
}


} } } }
