/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2015, KR2013ApS
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of the KR2013ApS nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *********************************************************************/

#include <kord/polynomials/from_constraints.h>

#include <cmath>

namespace kswx_weaving_generator
{
///////////////////////////////////////////////////////////////////////////////
// class PolynomialFromConstraints_C1At2Points_01                            //
///////////////////////////////////////////////////////////////////////////////
    PolynomialFromConstraints_C1At2Points_01
                ::PolynomialFromConstraints_C1At2Points_01()
    : v0_(NAN), v1_(NAN), d0_(NAN), d1_(NAN)
    {}
    
    PolynomialFromConstraints_C1At2Points_01& PolynomialFromConstraints_C1At2Points_01
                ::setValues(double a_v0, double a_v1)
    {
        v0_ = a_v0;
        v1_ = a_v1;
        return *this;
    }
    
    PolynomialFromConstraints_C1At2Points_01& PolynomialFromConstraints_C1At2Points_01
                ::setDerivatives(double a_d0, double a_d1)
    {
        d0_ = a_d0;
        d1_ = a_d1;
        return *this;
    }

    Degree3Polynomial PolynomialFromConstraints_C1At2Points_01
                ::compute() const
    {
        double a = v0_;
        double b = d0_;
        double c =  3. * (v1_ - v0_) - 2 * d0_ - d1_;
        double d = -2. * (v1_ - v0_) +     d0_ + d1_;
        return Degree3Polynomial(a, b, c, d);
    }

///////////////////////////////////////////////////////////////////////////////
// class PolynomialFromConstraints_C1At2Points_0A                            //
///////////////////////////////////////////////////////////////////////////////
    PolynomialFromConstraints_C1At2Points_0A
                ::PolynomialFromConstraints_C1At2Points_0A()
    : p1_(NAN)
    {}

    PolynomialFromConstraints_C1At2Points_0A& PolynomialFromConstraints_C1At2Points_0A
                ::setPoint(double a_p1)
    {
        p1_ = a_p1;
        return *this;
    }

    PolynomialFromConstraints_C1At2Points_0A& PolynomialFromConstraints_C1At2Points_0A
                ::setValues(double a_v0, double a_v1)
    {
        parent_.setValues(a_v0, a_v1);
        return *this;
    }

    PolynomialFromConstraints_C1At2Points_0A& PolynomialFromConstraints_C1At2Points_0A
                ::setDerivatives(double a_d0, double a_d1)
    {
        parent_.setDerivatives(a_d0 * p1_, a_d1 * p1_);
        return *this;
    }

    Degree3Polynomial PolynomialFromConstraints_C1At2Points_0A
                ::compute() const
    {
        Degree3Polynomial result = parent_.compute();
        result.xStretch(p1_);
        return result;
    }

///////////////////////////////////////////////////////////////////////////////
// class PolynomialFromConstraints_C1At2Points_AA                            //
///////////////////////////////////////////////////////////////////////////////
    PolynomialFromConstraints_C1At2Points_AA
                ::PolynomialFromConstraints_C1At2Points_AA()
    : p1_(NAN)
    {}
    
    PolynomialFromConstraints_C1At2Points_AA& PolynomialFromConstraints_C1At2Points_AA
                ::setPoints(double a_p0, double a_p1)
    {
        p0_ = a_p0;
        p1_ = a_p1;
        return *this;
    }
    
    PolynomialFromConstraints_C1At2Points_AA& PolynomialFromConstraints_C1At2Points_AA
                ::setValues(double a_v0, double a_v1)
    {
        parent_.setValues(a_v0, a_v1);
        return *this;
    }

    PolynomialFromConstraints_C1At2Points_AA& PolynomialFromConstraints_C1At2Points_AA
                ::setDerivatives(double a_d0, double a_d1)
    {
        parent_.setDerivatives(a_d0 * (p1_ - p0_), a_d1 * (p1_ - p0_));
        return *this;
    }

    Degree3Polynomial PolynomialFromConstraints_C1At2Points_AA
                ::compute() const
    {
        Degree3Polynomial result = parent_.compute();
        result.xStretch(p1_-p0_);
        result.xShift(-p0_);
        return result;
    }

///////////////////////////////////////////////////////////////////////////////
// class PolynomialFromConstraints_C2At2Points_01                            //
///////////////////////////////////////////////////////////////////////////////
    PolynomialFromConstraints_C2At2Points_01
                ::PolynomialFromConstraints_C2At2Points_01()
    : v0_(NAN), v1_(NAN), d0_(NAN), d1_(NAN)
    {}
    
    PolynomialFromConstraints_C2At2Points_01& PolynomialFromConstraints_C2At2Points_01
                ::setValues(double a_v0, double a_v1)
    {
        v0_ = a_v0;
        v1_ = a_v1;
        return *this;
    }
    
    PolynomialFromConstraints_C2At2Points_01& PolynomialFromConstraints_C2At2Points_01
                ::setDerivatives(double a_d0, double a_d1)
    {
        d0_ = a_d0;
        d1_ = a_d1;
        return *this;
    }

    PolynomialFromConstraints_C2At2Points_01& PolynomialFromConstraints_C2At2Points_01
                ::setSecondDerivatives(double a_s0, double a_s1)
    {
        s0_ = a_s0;
        s1_ = a_s1;
        return *this;
    }

    Degree5Polynomial PolynomialFromConstraints_C2At2Points_01
                ::compute() const
    {
        double a =       v0_;
        double b =                 d0_;
        double c =                       0.5 * s0_;
        double d = -10 * v0_ - 6 * d0_ - 1.5 * s0_ + 10 * v1_ - 4 * d1_ + 0.5 * s1_;
        double e =  15 * v0_ + 8 * d0_ + 1.5 * s0_ - 15 * v1_ + 7 * d1_ - 1   * s1_;
        double f =  -6 * v0_ - 3 * d0_ - 0.5 * s0_ +  6 * v1_ - 3 * d1_ + 0.5 * s1_;
        return Degree5Polynomial(a, b, c, d, e, f);
    }

///////////////////////////////////////////////////////////////////////////////
// class PolynomialFromConstraints_C2At2Points_0A                            //
///////////////////////////////////////////////////////////////////////////////
    PolynomialFromConstraints_C2At2Points_0A
                ::PolynomialFromConstraints_C2At2Points_0A()
    : p1_(NAN)
    {}

    PolynomialFromConstraints_C2At2Points_0A& PolynomialFromConstraints_C2At2Points_0A
                ::setPoint(double a_p1)
    {
        p1_ = a_p1;
        return *this;
    }

    PolynomialFromConstraints_C2At2Points_0A& PolynomialFromConstraints_C2At2Points_0A
                ::setValues(double a_v0, double a_v1)
    {
        parent_.setValues(a_v0, a_v1);
        return *this;
    }

    PolynomialFromConstraints_C2At2Points_0A& PolynomialFromConstraints_C2At2Points_0A
                ::setDerivatives(double a_d0, double a_d1)
    {
        parent_.setDerivatives(a_d0 * p1_, a_d1 * p1_);
        return *this;
    }

    PolynomialFromConstraints_C2At2Points_0A& PolynomialFromConstraints_C2At2Points_0A
                ::setSecondDerivatives(double a_s0, double a_s1)
    {
        parent_.setSecondDerivatives(a_s0 * p1_ * p1_, a_s1 * p1_ * p1_);
        return *this;
    }

    Degree5Polynomial PolynomialFromConstraints_C2At2Points_0A
                ::compute() const
    {
        Degree5Polynomial result = parent_.compute();
        result.xStretch(p1_);
        return result;
    }

///////////////////////////////////////////////////////////////////////////////
// class PolynomialFromConstraints_C2At2Points_AA                            //
///////////////////////////////////////////////////////////////////////////////
    PolynomialFromConstraints_C2At2Points_AA
                ::PolynomialFromConstraints_C2At2Points_AA()
    : p1_(NAN)
    {}
    
    PolynomialFromConstraints_C2At2Points_AA& PolynomialFromConstraints_C2At2Points_AA
                ::setPoints(double a_p0, double a_p1)
    {
        p0_ = a_p0;
        p1_ = a_p1;
        return *this;
    }
    
    PolynomialFromConstraints_C2At2Points_AA& PolynomialFromConstraints_C2At2Points_AA
                ::setValues(double a_v0, double a_v1)
    {
        parent_.setValues(a_v0, a_v1);
        return *this;
    }

    PolynomialFromConstraints_C2At2Points_AA& PolynomialFromConstraints_C2At2Points_AA
                ::setDerivatives(double a_d0, double a_d1)
    {
        parent_.setDerivatives(a_d0 * (p1_ - p0_), a_d1 * (p1_ - p0_));
        return *this;
    }

    PolynomialFromConstraints_C2At2Points_AA& PolynomialFromConstraints_C2At2Points_AA
                ::setSecondDerivatives(double a_s0, double a_s1)
    {
        parent_.setSecondDerivatives(a_s0 * (p1_ - p0_) * (p1_ - p0_), a_s1 * (p1_ - p0_) * (p1_ - p0_));
        return *this;
    }

    Degree5Polynomial PolynomialFromConstraints_C2At2Points_AA
                ::compute() const
    {
        Degree5Polynomial result = parent_.compute();
        result.xStretch(p1_-p0_);
        result.xShift(-p0_);
        return result;
    }

///////////////////////////////////////////////////////////////////////////////
// class PolynomialFromConstraints_C3At2Points_01                            //
///////////////////////////////////////////////////////////////////////////////
    PolynomialFromConstraints_C3At2Points_01
                ::PolynomialFromConstraints_C3At2Points_01()
    : v0_(NAN), v1_(NAN), d0_(NAN), d1_(NAN)
    {}
    
    PolynomialFromConstraints_C3At2Points_01& PolynomialFromConstraints_C3At2Points_01
                ::setValues(double a_v0, double a_v1)
    {
        v0_ = a_v0;
        v1_ = a_v1;
        return *this;
    }
    
    PolynomialFromConstraints_C3At2Points_01& PolynomialFromConstraints_C3At2Points_01
                ::setDerivatives(double a_d0, double a_d1)
    {
        d0_ = a_d0;
        d1_ = a_d1;
        return *this;
    }

    PolynomialFromConstraints_C3At2Points_01& PolynomialFromConstraints_C3At2Points_01
                ::setSecondDerivatives(double a_s0, double a_s1)
    {
        s0_ = a_s0;
        s1_ = a_s1;
        return *this;
    }

    PolynomialFromConstraints_C3At2Points_01& PolynomialFromConstraints_C3At2Points_01
                ::setThirdDerivatives(double a_t0, double a_t1)
    {
        t0_ = a_t0;
        t1_ = a_t1;
        return *this;
    }

    Degree7Polynomial PolynomialFromConstraints_C3At2Points_01
                ::compute() const
    {
        double a =       v0_;
        double b =                  d0_;
        double c =                         0.5 * s0_;
        double d =                                     1./6 * t0_;
        double e = -35 * v0_ - 20 * d0_ -  5   * s0_ - 2./3 * t0_ + 35 * v1_ - 15 * d1_ + 2.5 * s1_ - 1./6 * t1_;
        double f =  84 * v0_ + 45 * d0_ + 10   * s0_ +   1  * t0_ - 84 * v1_ + 39 * d1_ - 7   * s1_ + 1./2 * t1_;
        double g = -70 * v0_ - 36 * d0_ -  7.5 * s0_ - 2./3 * t0_ + 70 * v1_ - 34 * d1_ + 6.5 * s1_ - 1./2 * t1_;
        double h =  20 * v0_ + 10 * d0_ +  2   * s0_ + 1./6 * t0_ - 20 * v1_ + 10 * d1_ - 2   * s1_ + 1./6 * t1_;
        return Degree7Polynomial(a, b, c, d, e, f, g, h);
    }

///////////////////////////////////////////////////////////////////////////////
// class PolynomialFromConstraints_C3At2Points_0A                            //
///////////////////////////////////////////////////////////////////////////////
    PolynomialFromConstraints_C3At2Points_0A
                ::PolynomialFromConstraints_C3At2Points_0A()
    : p1_(NAN)
    {}

    PolynomialFromConstraints_C3At2Points_0A& PolynomialFromConstraints_C3At2Points_0A
                ::setPoint(double a_p1)
    {
        p1_ = a_p1;
        return *this;
    }

    PolynomialFromConstraints_C3At2Points_0A& PolynomialFromConstraints_C3At2Points_0A
                ::setValues(double a_v0, double a_v1)
    {
        parent_.setValues(a_v0, a_v1);
        return *this;
    }

    PolynomialFromConstraints_C3At2Points_0A& PolynomialFromConstraints_C3At2Points_0A
                ::setDerivatives(double a_d0, double a_d1)
    {
        parent_.setDerivatives(a_d0 * p1_, a_d1 * p1_);
        return *this;
    }

    PolynomialFromConstraints_C3At2Points_0A& PolynomialFromConstraints_C3At2Points_0A
                ::setSecondDerivatives(double a_s0, double a_s1)
    {
        parent_.setSecondDerivatives(a_s0 * p1_ * p1_, a_s1 * p1_ * p1_);
        return *this;
    }

    PolynomialFromConstraints_C3At2Points_0A& PolynomialFromConstraints_C3At2Points_0A
                ::setThirdDerivatives(double a_t0, double a_t1)
    {
        parent_.setThirdDerivatives(a_t0 * p1_ * p1_ * p1_, a_t1 * p1_ * p1_ * p1_);
        return *this;
    }

    Degree7Polynomial PolynomialFromConstraints_C3At2Points_0A
                ::compute() const
    {
        if (p1_ > 1e-8) {
            Degree7Polynomial result = parent_.compute();
            result.xStretch(p1_);
            return result;
        } else {
            return Degree7Polynomial(parent_.v0(), 2. * parent_.d0(), 6. * parent_.s0(), 24. * parent_.t0(), 0., 0., 0., 0.);
        }
    }

///////////////////////////////////////////////////////////////////////////////
// class PolynomialFromConstraints_C3At2Points_AA                            //
///////////////////////////////////////////////////////////////////////////////
    PolynomialFromConstraints_C3At2Points_AA
                ::PolynomialFromConstraints_C3At2Points_AA()
    : p1_(NAN)
    {}
    
    PolynomialFromConstraints_C3At2Points_AA& PolynomialFromConstraints_C3At2Points_AA
                ::setPoints(double a_p0, double a_p1)
    {
        p0_ = a_p0;
        p1_ = a_p1;
        return *this;
    }
    
    PolynomialFromConstraints_C3At2Points_AA& PolynomialFromConstraints_C3At2Points_AA
                ::setValues(double a_v0, double a_v1)
    {
        parent_.setValues(a_v0, a_v1);
        return *this;
    }

    PolynomialFromConstraints_C3At2Points_AA& PolynomialFromConstraints_C3At2Points_AA
                ::setDerivatives(double a_d0, double a_d1)
    {
        parent_.setDerivatives(a_d0 * (p1_ - p0_), a_d1 * (p1_ - p0_));
        return *this;
    }

    PolynomialFromConstraints_C3At2Points_AA& PolynomialFromConstraints_C3At2Points_AA
                ::setSecondDerivatives(double a_s0, double a_s1)
    {
        parent_.setSecondDerivatives(a_s0 * (p1_ - p0_) * (p1_ - p0_), a_s1 * (p1_ - p0_) * (p1_ - p0_));
        return *this;
    }

    PolynomialFromConstraints_C3At2Points_AA& PolynomialFromConstraints_C3At2Points_AA
                ::setThirdDerivatives(double a_s0, double a_s1)
    {
        parent_.setThirdDerivatives(a_s0 * (p1_ - p0_) * (p1_ - p0_) * (p1_ - p0_), a_s1 * (p1_ - p0_) * (p1_ - p0_) * (p1_ - p0_));
        return *this;
    }

    Degree7Polynomial PolynomialFromConstraints_C3At2Points_AA
                ::compute() const
    {
        Degree7Polynomial result = parent_.compute();
        result.xStretch(p1_-p0_);
        result.xShift(-p0_);
        return result;
    }

}; // namespace
