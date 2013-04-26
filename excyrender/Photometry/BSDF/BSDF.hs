-- (C) 2013 Sebastian Mach (1983), this file is published under the terms of the
-- GNU General Public License, Version 3 (a.k.a. GPLv3).
-- See COPYING in the root-folder of the excygen project folder.

module Photometry.BSDF.BSDF
( BSDF(..)
) where

import Geometry.Direction
import Photometry.SPD.SPD

data BSDF t = BSDF {
    pdf :: Direction t -> Direction t -> t,
    f   :: Direction t -> Direction t -> SPD t
}
