-- (C) 2013 Sebastian Mach (1983), this file is published under the terms of the
-- GNU General Public License, Version 3 (a.k.a. GPLv3).
-- See COPYING in the root-folder of the excygen project folder.

module Intersection
( distance
, Intersection(..)
) where

import RGB(RGB(..))



-- Distance --------------------------------------------------------------------
data Distance t = Distance t
                  deriving (Show)

distance :: (Num t, Ord t, Fractional t) => t -> Distance t

distance f | f>=0      = Distance f
           | otherwise = error "Distance can't be negative"



-- Intersection ----------------------------------------------------------------
data Intersection t = Intersection {
    d :: Distance t,
    diffuse :: RGB t
} deriving (Show)

