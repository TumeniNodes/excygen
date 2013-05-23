-- (C) 2013 Sebastian Mach (1983), this file is published under the terms of the
-- GNU General Public License, Version 3 (a.k.a. GPLv3).
-- See COPYING in the root-folder of the excygen project folder.

module Photometry.BSDF.BSDF
( BSDF, bsdf,
  f, pdf, sample_f -- TODO: sample_f should probably take a DifferentialGeometry
) where

import qualified Photometry.Spectrum as Sp
import RealNum
import qualified Photometry.BSDF.BxDF as X 

import Geometry.Direction as D
import DifferentialGeometry


-- interface --------------------------------------------------------------------------------------
data BSDF = BSDF [X.BxDF]

bsdf     :: [X.BxDF] -> BSDF
pdf      :: BSDF -> Direction -> Direction -> RealNum
f        :: BSDF -> Direction -> Direction -> Sp.Spectrum
sample_f :: BSDF -> DifferentialGeometry -> Direction -> (Direction, Sp.Spectrum, RealNum) -- TODO: those should all return Maybe
            -- Note: For sample_f, the direction points away of the point of intersection.



-- implementation ---------------------------------------------------------------------------------
bsdf [] = error "BSDF must have one or more BxDFs"
bsdf xs = BSDF xs


f (BSDF xs) wo wi = Sp.sum
                  . map (\bxdf -> X.f bxdf wo wi)
                  . continuous $ xs


pdf (BSDF xs) wo wi = sum
                    . map (\bxdf -> (X.pdf bxdf wo wi))
                    . continuous $ xs


sample_f (BSDF xs) dg wo =
   let bxdfs = specular xs
       wo' = (worldToLocalDirection dg) wo
   in if null bxdfs then (direction 0 1 0, Sp.spectrum 100 600 [0], 0)
      else if length bxdfs /= 1 then error "BSDF currently supports up to 1 specular BxDFs"
      else let bxdf = head bxdfs
               (dir, spec, pdf') = X.sample_f bxdf wo'
           in ((localToWorldDirection dg) dir, spec, pdf')



-- internal ---------------------------------------------------------------------------------------
continuous :: [X.BxDF] -> [X.BxDF]
specular   :: [X.BxDF] -> [X.BxDF]
continuous = filter (\bxdf -> X.distribution bxdf == X.Continuous)
specular   = filter (\bxdf -> X.distribution bxdf == X.Specular)

