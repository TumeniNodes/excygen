-- (C) 2013 Sebastian Mach (1983), this file is published under the terms of the
-- GNU General Public License, Version 3 (a.k.a. GPLv3).
-- See COPYING in the root-folder of the excygen project folder.

module Geometry
( Angle
) where

-- Angle -----------------------------------------------------------------------
data Angle t = Degrees t | Radians t

instance (Show t) => Show (Angle t) where
    show (Degrees f) = show f ++ "°"
    show (Radians f) = show f ++ " rad"

degrees    :: (Floating t) => Angle t -> t
radians    :: (Floating t) => Angle t -> t
as_degrees :: (Floating t) => (Angle t) -> (Angle t)
as_radians :: (Floating t) => (Angle t) -> (Angle t)

degrees (Degrees val) = val
degrees (Radians val) = val * (180/pi)
radians (Degrees val) = val * (pi/180)
radians (Radians val) = val
as_degrees angle = Degrees $ degrees angle 
as_radians angle = Radians $ radians angle 

-- Vector ----------------------------------------------------------------------
data Vector t = Vector t t t
                deriving (Show)

v_add       :: (Num t)        => Vector t -> Vector t -> Vector t
v_sub       :: (Num t)        => Vector t -> Vector t -> Vector t
v_len_sq    :: (Num t)        => Vector t -> t
v_len       :: (Floating t)   => Vector t -> t
v_stretch   :: (Num t)        => Vector t -> t -> Vector t
v_shrink    :: (Fractional t) => Vector t -> t -> Vector t
v_normalize :: (Floating t)   => Vector t -> Vector t

v_add       (Vector a b c) (Vector x y z) = Vector (a+x) (b+y) (c+z) 
v_sub       (Vector a b c) (Vector x y z) = Vector (a-x) (b-y) (c-z) 
v_len_sq    (Vector a b c)                = a*a + b*b + c*c
v_len                                     = sqrt . v_len_sq
v_stretch   (Vector a b c) f              = Vector (a*f) (b*f) (c*f)
v_shrink    (Vector a b c) f              = Vector (a/f) (b/f) (c/f)
v_normalize v                             = v `v_shrink` (v_len v)

-- Point -----------------------------------------------------------------------
data Point t = Point t t t
               deriving (Show)

p_add  :: (Num t) => Point t -> Vector t -> Point  t
p_sub  :: (Num t) => Point t -> Vector t -> Point  t
p_diff :: (Num t) => Point t -> Point  t -> Vector t

p_add  (Point a b c) (Vector x y z) = Point  (a+x) (b+y) (c+z)
p_sub  (Point a b c) (Vector x y z) = Point  (a-x) (b-y) (c-z)
p_diff (Point a b c) (Point  x y z) = Vector (a-x) (b-y) (c-z)
