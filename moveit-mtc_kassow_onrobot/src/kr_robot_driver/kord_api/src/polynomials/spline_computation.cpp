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

#include <kord/polynomials/spline_computation.h>

// #define DEBUG_PRINT_SPLINE_COMPUTATION
#ifdef DEBUG_PRINT_SPLINE_COMPUTATION
#include <iostream>
#endif

namespace kswx_weaving_generator
{
    
int fnGetEnvIntWithDefault(const char* a_var_name, int a_default_value)
{
    auto value = std::getenv(a_var_name);
    if (value) {
        int result = atoi(value);
        if (result != 0) {
            return result;
        }

        return a_default_value;
    }

    return a_default_value;

}

///////////////////////////////////////////////////////////////////////////////
// class SplineC2S6                                                          //
///////////////////////////////////////////////////////////////////////////////
    SplineC2S6
                ::SplineC2S6()
    : s3c_()
    , s3p0_(s3c_.addPolynomial(fnGetEnvIntWithDefault("ST_C2S6_D0", 4)))
    , s3p1_(s3c_.addPolynomial(fnGetEnvIntWithDefault("ST_C2S6_D1", 3)))
    , s3p2_(s3c_.addPolynomial(fnGetEnvIntWithDefault("ST_C2S6_D2", 3)))
    , s3p3_(s3c_.addPolynomial(fnGetEnvIntWithDefault("ST_C2S6_D3", 3)))
    , s3p4_(s3c_.addPolynomial(fnGetEnvIntWithDefault("ST_C2S6_D4", 3)))
    , s3p5_(s3c_.addPolynomial(fnGetEnvIntWithDefault("ST_C2S6_D5", 4)))
    , s3e_(s3c_)
    , y0_(NAN), y1_(NAN), y2_(NAN), y3_(NAN), y4_(NAN), y5_(NAN), y6_(NAN)
    , h0_(NAN), h1_(NAN), h2_(NAN), h3_(NAN), h4_(NAN), h5_(NAN)
    , d0_(NAN), d6_(NAN), s0_(NAN), s6_(NAN)
    {}

    void SplineC2S6
                ::setXs(double a_x0, double a_x1, double a_x2, double a_x3, double a_x4, double a_x5, double a_x6)
    {
        h0_ = a_x1 - a_x0;
        h1_ = a_x2 - a_x1;
        h2_ = a_x3 - a_x2;
        h3_ = a_x4 - a_x3;
        h4_ = a_x5 - a_x4;
        h5_ = a_x6 - a_x5;
    }

    void SplineC2S6
                ::setYs(double a_y0, double a_y1, double a_y2, double a_y3, double a_y4, double a_y5, double a_y6)
    {
        y0_ = a_y0;
        y1_ = a_y1;
        y2_ = a_y2;
        y3_ = a_y3;
        y4_ = a_y4;
        y5_ = a_y5;
        y6_ = a_y6;
#ifdef DEBUG_PRINT_SPLINE_COMPUTATION
        std::cout << "Spline computation: " << y0_ << " ~ " << y1_ << " ~ " << y2_ << " ~ " << y3_ << " ~ " << y4_ << " ~ " << y5_ << " ~ " << y6_ << std::endl;
#endif
    }

    void SplineC2S6
                ::setStartConds(double a_d0, double a_s0)
    {
        d0_ = a_d0;
        s0_ = a_s0;
    }

    void SplineC2S6
                ::setEndConds(double a_d6, double a_s6)
    {
        d6_ = a_d6;
        s6_ = a_s6;
    }

    void SplineC2S6
                ::computeAndSaveTo(std::vector<Degree5Polynomial>& results)
    {
        s3e_.clear();
        s3e_.addEquation1(s3p0_.valueAt(0.), y0_);
        s3e_.addEquation1(s3p0_.valueAt(h0_), y1_);
        s3e_.addEquation1(s3p0_.derivativeAt(0.), d0_);
        s3e_.addEquation2(s3p0_.derivativeAt(h0_), s3p1_.derivativeAt(0.));
        s3e_.addEquation1(s3p0_.secondDerivativeAt(0.), s0_);
        s3e_.addEquation2(s3p0_.secondDerivativeAt(h0_), s3p1_.secondDerivativeAt(0.));
        s3e_.addEquation1(s3p1_.valueAt(0.), y1_);
        s3e_.addEquation1(s3p1_.valueAt(h1_), y2_);
        s3e_.addEquation2(s3p1_.derivativeAt(h1_), s3p2_.derivativeAt(0.));
        s3e_.addEquation2(s3p1_.secondDerivativeAt(h1_), s3p2_.secondDerivativeAt(0.));
        s3e_.addEquation1(s3p2_.valueAt(0.), y2_);
        s3e_.addEquation1(s3p2_.valueAt(h2_), y3_);
        s3e_.addEquation2(s3p2_.derivativeAt(h2_), s3p3_.derivativeAt(0.));
        s3e_.addEquation2(s3p2_.secondDerivativeAt(h2_), s3p3_.secondDerivativeAt(0.));
        s3e_.addEquation1(s3p3_.valueAt(0.), y3_);
        s3e_.addEquation1(s3p3_.valueAt(h3_), y4_);
        s3e_.addEquation2(s3p3_.derivativeAt(h3_), s3p4_.derivativeAt(0.));
        s3e_.addEquation2(s3p3_.secondDerivativeAt(h3_), s3p4_.secondDerivativeAt(0.));
        s3e_.addEquation1(s3p4_.valueAt(0.), y4_);
        s3e_.addEquation1(s3p4_.valueAt(h4_), y5_);
        s3e_.addEquation2(s3p4_.derivativeAt(h4_), s3p5_.derivativeAt(0.));
        s3e_.addEquation2(s3p4_.secondDerivativeAt(h4_), s3p5_.secondDerivativeAt(0.));
        s3e_.addEquation1(s3p5_.valueAt(0.), y5_);
        s3e_.addEquation1(s3p5_.valueAt(h5_), y6_);
        s3e_.addEquation1(s3p5_.derivativeAt(h5_), d6_);
        s3e_.addEquation1(s3p5_.secondDerivativeAt(h5_), s6_);
        s3e_.solve();
        results.push_back(s3e_.buildDegree5Polynomial(s3p0_));
        results.push_back(s3e_.buildDegree5Polynomial(s3p1_));
        results.push_back(s3e_.buildDegree5Polynomial(s3p2_));
        results.push_back(s3e_.buildDegree5Polynomial(s3p3_));
        results.push_back(s3e_.buildDegree5Polynomial(s3p4_));
        results.push_back(s3e_.buildDegree5Polynomial(s3p5_));
    }

///////////////////////////////////////////////////////////////////////////////
// class SplineC2S5                                                          //
///////////////////////////////////////////////////////////////////////////////
    SplineC2S5
                ::SplineC2S5()
    : s3c_()
    , s3p0_(s3c_.addPolynomial(fnGetEnvIntWithDefault("ST_C2S5_D0", 4)))
    , s3p1_(s3c_.addPolynomial(fnGetEnvIntWithDefault("ST_C2S5_D1", 3)))
    , s3p2_(s3c_.addPolynomial(fnGetEnvIntWithDefault("ST_C2S5_D2", 3)))
    , s3p3_(s3c_.addPolynomial(fnGetEnvIntWithDefault("ST_C2S5_D3", 3)))
    , s3p4_(s3c_.addPolynomial(fnGetEnvIntWithDefault("ST_C2S5_D4", 4)))
    , s3e_(s3c_)
    , y0_(NAN), y1_(NAN), y2_(NAN), y3_(NAN), y4_(NAN), y5_(NAN)
    , h0_(NAN), h1_(NAN), h2_(NAN), h3_(NAN), h4_(NAN)
    , d0_(NAN), d5_(NAN), s0_(NAN), s5_(NAN)
    {}

    void SplineC2S5
                ::setXs(double a_x0, double a_x1, double a_x2, double a_x3, double a_x4, double a_x5)
    {
        h0_ = a_x1 - a_x0;
        h1_ = a_x2 - a_x1;
        h2_ = a_x3 - a_x2;
        h3_ = a_x4 - a_x3;
        h4_ = a_x5 - a_x4;
    }

    void SplineC2S5
                ::setYs(double a_y0, double a_y1, double a_y2, double a_y3, double a_y4, double a_y5)
    {
        y0_ = a_y0;
        y1_ = a_y1;
        y2_ = a_y2;
        y3_ = a_y3;
        y4_ = a_y4;
        y5_ = a_y5;
#ifdef DEBUG_PRINT_SPLINE_COMPUTATION
        std::cout << "Spline computation: " << y0_ << " ~ " << y1_ << " ~ " << y2_ << " ~ " << y3_ << " ~ " << y4_ << " ~ " << y5_ << std::endl;
#endif
    }

    void SplineC2S5
                ::setStartConds(double a_d0, double a_s0)
    {
        d0_ = a_d0;
        s0_ = a_s0;
    }

    void SplineC2S5
                ::setEndConds(double a_d5, double a_s5)
    {
        d5_ = a_d5;
        s5_ = a_s5;
    }

    void SplineC2S5
                ::computeAndSaveTo(std::vector<Degree5Polynomial>& results)
    {
        s3e_.clear();
        s3e_.addEquation1(s3p0_.valueAt(0.), y0_);
        s3e_.addEquation1(s3p0_.valueAt(h0_), y1_);
        s3e_.addEquation1(s3p0_.derivativeAt(0.), d0_);
        s3e_.addEquation2(s3p0_.derivativeAt(h0_), s3p1_.derivativeAt(0.));
        s3e_.addEquation1(s3p0_.secondDerivativeAt(0.), s0_);
        s3e_.addEquation2(s3p0_.secondDerivativeAt(h0_), s3p1_.secondDerivativeAt(0.));
        s3e_.addEquation1(s3p1_.valueAt(0.), y1_);
        s3e_.addEquation1(s3p1_.valueAt(h1_), y2_);
        s3e_.addEquation2(s3p1_.derivativeAt(h1_), s3p2_.derivativeAt(0.));
        s3e_.addEquation2(s3p1_.secondDerivativeAt(h1_), s3p2_.secondDerivativeAt(0.));
        s3e_.addEquation1(s3p2_.valueAt(0.), y2_);
        s3e_.addEquation1(s3p2_.valueAt(h2_), y3_);
        s3e_.addEquation2(s3p2_.derivativeAt(h2_), s3p3_.derivativeAt(0.));
        s3e_.addEquation2(s3p2_.secondDerivativeAt(h2_), s3p3_.secondDerivativeAt(0.));
        s3e_.addEquation1(s3p3_.valueAt(0.), y3_);
        s3e_.addEquation1(s3p3_.valueAt(h3_), y4_);
        s3e_.addEquation2(s3p3_.derivativeAt(h3_), s3p4_.derivativeAt(0.));
        s3e_.addEquation2(s3p3_.secondDerivativeAt(h3_), s3p4_.secondDerivativeAt(0.));
        s3e_.addEquation1(s3p4_.valueAt(0.), y4_);
        s3e_.addEquation1(s3p4_.valueAt(h4_), y5_);
        s3e_.addEquation1(s3p4_.derivativeAt(h4_), d5_);
        s3e_.addEquation1(s3p4_.secondDerivativeAt(h4_), s5_);
        s3e_.solve();
        results.push_back(s3e_.buildDegree5Polynomial(s3p0_));
        results.push_back(s3e_.buildDegree5Polynomial(s3p1_));
        results.push_back(s3e_.buildDegree5Polynomial(s3p2_));
        results.push_back(s3e_.buildDegree5Polynomial(s3p3_));
        results.push_back(s3e_.buildDegree5Polynomial(s3p4_));
    }

///////////////////////////////////////////////////////////////////////////////
// class SplineC2S4                                                          //
///////////////////////////////////////////////////////////////////////////////
    SplineC2S4
                ::SplineC2S4()
    : s3c_()
    , s3p0_(s3c_.addPolynomial(fnGetEnvIntWithDefault("ST_C2S4_D0", 4)))
    , s3p1_(s3c_.addPolynomial(fnGetEnvIntWithDefault("ST_C2S4_D1", 3)))
    , s3p2_(s3c_.addPolynomial(fnGetEnvIntWithDefault("ST_C2S4_D2", 3)))
    , s3p3_(s3c_.addPolynomial(fnGetEnvIntWithDefault("ST_C2S4_D3", 4)))
    , s3e_(s3c_)
    , y0_(NAN), y1_(NAN), y2_(NAN), y3_(NAN), y4_(NAN)
    , h0_(NAN), h1_(NAN), h2_(NAN), h3_(NAN)
    , d0_(NAN), d4_(NAN), s0_(NAN), s4_(NAN)
    {}

    void SplineC2S4
                ::setXs(double a_x0, double a_x1, double a_x2, double a_x3, double a_x4)
    {
        h0_ = a_x1 - a_x0;
        h1_ = a_x2 - a_x1;
        h2_ = a_x3 - a_x2;
        h3_ = a_x4 - a_x3;
    }

    void SplineC2S4
                ::setYs(double a_y0, double a_y1, double a_y2, double a_y3, double a_y4)
    {
        y0_ = a_y0;
        y1_ = a_y1;
        y2_ = a_y2;
        y3_ = a_y3;
        y4_ = a_y4;
#ifdef DEBUG_PRINT_SPLINE_COMPUTATION
        std::cout << "Spline computation: " << y0_ << " ~ " << y1_ << " ~ " << y2_ << " ~ " << y3_ << " ~ " << y4_ << std::endl;
#endif
    }

    void SplineC2S4
                ::setStartConds(double a_d0, double a_s0)
    {
        d0_ = a_d0;
        s0_ = a_s0;
    }

    void SplineC2S4
                ::setEndConds(double a_d4, double a_s4)
    {
        d4_ = a_d4;
        s4_ = a_s4;
    }

    void SplineC2S4
                ::computeAndSaveTo(std::vector<Degree5Polynomial>& results)
    {
        s3e_.clear();
        s3e_.addEquation1(s3p0_.valueAt(0.), y0_);
        s3e_.addEquation1(s3p0_.valueAt(h0_), y1_);
        s3e_.addEquation1(s3p0_.derivativeAt(0.), d0_);
        s3e_.addEquation2(s3p0_.derivativeAt(h0_), s3p1_.derivativeAt(0.));
        s3e_.addEquation1(s3p0_.secondDerivativeAt(0.), s0_);
        s3e_.addEquation2(s3p0_.secondDerivativeAt(h0_), s3p1_.secondDerivativeAt(0.));
        s3e_.addEquation1(s3p1_.valueAt(0.), y1_);
        s3e_.addEquation1(s3p1_.valueAt(h1_), y2_);
        s3e_.addEquation2(s3p1_.derivativeAt(h1_), s3p2_.derivativeAt(0.));
        s3e_.addEquation2(s3p1_.secondDerivativeAt(h1_), s3p2_.secondDerivativeAt(0.));
        s3e_.addEquation1(s3p2_.valueAt(0.), y2_);
        s3e_.addEquation1(s3p2_.valueAt(h2_), y3_);
        s3e_.addEquation2(s3p2_.derivativeAt(h2_), s3p3_.derivativeAt(0.));
        s3e_.addEquation2(s3p2_.secondDerivativeAt(h2_), s3p3_.secondDerivativeAt(0.));
        s3e_.addEquation1(s3p3_.valueAt(0.), y3_);
        s3e_.addEquation1(s3p3_.valueAt(h3_), y4_);
        s3e_.addEquation1(s3p3_.derivativeAt(h3_), d4_);
        s3e_.addEquation1(s3p3_.secondDerivativeAt(h3_), s4_);
        s3e_.solve();
        results.push_back(s3e_.buildDegree5Polynomial(s3p0_));
        results.push_back(s3e_.buildDegree5Polynomial(s3p1_));
        results.push_back(s3e_.buildDegree5Polynomial(s3p2_));
        results.push_back(s3e_.buildDegree5Polynomial(s3p3_));
    }

///////////////////////////////////////////////////////////////////////////////
// class SplineC2S3                                                          //
///////////////////////////////////////////////////////////////////////////////

    SplineC2S3
                ::SplineC2S3()
    : s3c_()
    , s3p0_(s3c_.addPolynomial(fnGetEnvIntWithDefault("ST_C2S3_D0", 4)))
    , s3p1_(s3c_.addPolynomial(fnGetEnvIntWithDefault("ST_C2S3_D1", 3)))
    , s3p2_(s3c_.addPolynomial(fnGetEnvIntWithDefault("ST_C2S3_D2", 4)))
    , s3e_(s3c_)
    , y0_(NAN), y1_(NAN), y2_(NAN), y3_(NAN), h0_(NAN), h1_(NAN), h2_(NAN), d0_(NAN), d3_(NAN), s0_(NAN), s3_(NAN)
    {}

    void SplineC2S3
                ::setXs(double a_x0, double a_x1, double a_x2, double a_x3)
    {
        h0_ = a_x1 - a_x0;
        h1_ = a_x2 - a_x1;
        h2_ = a_x3 - a_x2;
    }

    void SplineC2S3
                ::setYs(double a_y0, double a_y1, double a_y2, double a_y3)
    {
        y0_ = a_y0;
        y1_ = a_y1;
        y2_ = a_y2;
        y3_ = a_y3;
#ifdef DEBUG_PRINT_SPLINE_COMPUTATION
        std::cout << "Spline computation: " << y0_ << " ~ " << y1_ << " ~ " << y2_ << " ~ " << y3_ << std::endl;
#endif
    }

    void SplineC2S3
                ::setStartConds(double a_d0, double a_s0)
    {
        d0_ = a_d0;
        s0_ = a_s0;
    }

    void SplineC2S3
                ::setEndConds(double a_d3, double a_s3)
    {
        d3_ = a_d3;
        s3_ = a_s3;
    }

    void SplineC2S3
                ::computeAndSaveTo(std::vector<Degree5Polynomial>& results)
    {
        s3e_.clear();
        s3e_.addEquation1(s3p0_.valueAt(0.), y0_);
        s3e_.addEquation1(s3p0_.valueAt(h0_), y1_);
        s3e_.addEquation1(s3p0_.derivativeAt(0.), d0_);
        s3e_.addEquation2(s3p0_.derivativeAt(h0_), s3p1_.derivativeAt(0.));
        s3e_.addEquation1(s3p0_.secondDerivativeAt(0.), s0_);
        s3e_.addEquation2(s3p0_.secondDerivativeAt(h0_), s3p1_.secondDerivativeAt(0.));
        s3e_.addEquation1(s3p1_.valueAt(0.), y1_);
        s3e_.addEquation1(s3p1_.valueAt(h1_), y2_);
        s3e_.addEquation2(s3p1_.derivativeAt(h1_), s3p2_.derivativeAt(0.));
        s3e_.addEquation2(s3p1_.secondDerivativeAt(h1_), s3p2_.secondDerivativeAt(0.));
        s3e_.addEquation1(s3p2_.valueAt(0.), y2_);
        s3e_.addEquation1(s3p2_.valueAt(h2_), y3_);
        s3e_.addEquation1(s3p2_.derivativeAt(h2_), d3_);
        s3e_.addEquation1(s3p2_.secondDerivativeAt(h2_), s3_);
        s3e_.solve();
        results.push_back(s3e_.buildDegree5Polynomial(s3p0_));
        results.push_back(s3e_.buildDegree5Polynomial(s3p1_));
        results.push_back(s3e_.buildDegree5Polynomial(s3p2_));
    }

///////////////////////////////////////////////////////////////////////////////
// class SplineC2S2                                                          //
///////////////////////////////////////////////////////////////////////////////
    SplineC2S2
                ::SplineC2S2()
    : s2c_()
    , s2p0_(s2c_.addPolynomial(fnGetEnvIntWithDefault("ST_C2S2_D0", 4)))
    , s2p1_(s2c_.addPolynomial(fnGetEnvIntWithDefault("ST_C2S2_D1", 4)))
    , s2e_(s2c_)
    , y0_(NAN), y1_(NAN), y2_(NAN), h0_(NAN), h1_(NAN), d0_(NAN), d2_(NAN), s0_(NAN), s2_(NAN)
    {}

    void SplineC2S2
                ::setXs(double a_x0, double a_x1, double a_x2)
    {
        h0_ = a_x1 - a_x0;
        h1_ = a_x2 - a_x1;
    }

    void SplineC2S2
                ::setYs(double a_y0, double a_y1, double a_y2)
    {
        y0_ = a_y0;
        y1_ = a_y1;
        y2_ = a_y2;
#ifdef DEBUG_PRINT_SPLINE_COMPUTATION
        std::cout << "Spline computation: " << y0_ << " ~ " << y1_ << " ~ " << y2_ << std::endl;
#endif
    }

    void SplineC2S2
                ::setStartConds(double a_d0, double a_s0)
    {
        d0_ = a_d0;
        s0_ = a_s0;
    }

    void SplineC2S2
                ::setEndConds(double a_d2, double a_s2)
    {
        d2_ = a_d2;
        s2_ = a_s2;
    }

    void SplineC2S2
                ::computeAndSaveTo(std::vector<Degree5Polynomial>& results)
    {
        s2e_.clear();
        s2e_.addEquation1(s2p0_.valueAt(0.), y0_);
        s2e_.addEquation1(s2p0_.valueAt(h0_), y1_);
        s2e_.addEquation1(s2p0_.derivativeAt(0.), d0_);
        s2e_.addEquation2(s2p0_.derivativeAt(h0_), s2p1_.derivativeAt(0.));
        s2e_.addEquation1(s2p0_.secondDerivativeAt(0.), s0_);
        s2e_.addEquation2(s2p0_.secondDerivativeAt(h0_), s2p1_.secondDerivativeAt(0.));
        s2e_.addEquation1(s2p1_.valueAt(0.), y1_);
        s2e_.addEquation1(s2p1_.valueAt(h1_), y2_);
        s2e_.addEquation1(s2p1_.derivativeAt(h1_), d2_);
        s2e_.addEquation1(s2p1_.secondDerivativeAt(h1_), s2_);
        s2e_.solve();
        results.push_back(s2e_.buildDegree5Polynomial(s2p0_));
        results.push_back(s2e_.buildDegree5Polynomial(s2p1_));
    }

///////////////////////////////////////////////////////////////////////////////
// class SplineC2S1                                                          //
///////////////////////////////////////////////////////////////////////////////

    SplineC2S1
                ::SplineC2S1()
    : s1c_()
    , s1p0_(s1c_.addPolynomial(fnGetEnvIntWithDefault("ST_C2S1_D0", 5)))
    , s1e_(s1c_)
    , y0_(NAN), y1_(NAN), h0_(NAN), d0_(NAN), d1_(NAN), s0_(NAN), s1_(NAN)
    {}

    void SplineC2S1
                ::setXs(double a_x0, double a_x1)
    {
        h0_ = a_x1 - a_x0;
    }

    void SplineC2S1
                ::setYs(double a_y0, double a_y1)
    {
        y0_ = a_y0;
        y1_ = a_y1;
#ifdef DEBUG_PRINT_SPLINE_COMPUTATION
        std::cout << "Spline computation: " << y0_ << " ~ " << y1_ << std::endl;
#endif
    }

    void SplineC2S1
                ::setStartConds(double a_d0, double a_s0)
    {
        d0_ = a_d0;
        s0_ = a_s0;
    }

    void SplineC2S1
                ::setEndConds(double a_d1, double a_s1)
    {
        d1_ = a_d1;
        s1_ = a_s1;
    }

    void SplineC2S1
                ::computeAndSaveTo(std::vector<Degree5Polynomial>& results)
    {
        s1e_.clear();
        s1e_.addEquation1(s1p0_.valueAt(0.), y0_);
        s1e_.addEquation1(s1p0_.valueAt(h0_), y1_);
        s1e_.addEquation1(s1p0_.derivativeAt(0.), d0_);
        s1e_.addEquation1(s1p0_.derivativeAt(h0_), d1_);
        s1e_.addEquation1(s1p0_.secondDerivativeAt(0.), s0_);
        s1e_.addEquation1(s1p0_.secondDerivativeAt(h0_), s1_);
        s1e_.solve();
        results.push_back(s1e_.buildDegree5Polynomial(s1p0_));
    }

///////////////////////////////////////////////////////////////////////////////
// class SplineC2aS4                                                         //
///////////////////////////////////////////////////////////////////////////////

    SplineC2aS4
                ::SplineC2aS4()
    : s3c_()
    , s3p0_(s3c_.addPolynomial(fnGetEnvIntWithDefault("ST_C2aS4_D0", 4)))
    , s3p1_(s3c_.addPolynomial(fnGetEnvIntWithDefault("ST_C2aS4_D1", 3)))
    , s3p2_(s3c_.addPolynomial(fnGetEnvIntWithDefault("ST_C2aS4_D2", 3)))
    , s3p3_(s3c_.addPolynomial(fnGetEnvIntWithDefault("ST_C2aS4_D3", 5)))
    , s3e_(s3c_)
    , y0_(NAN), y1_(NAN), y2_(NAN), y3_(NAN), y4_(NAN)
    , h0_(NAN), h1_(NAN), h2_(NAN), h3_(NAN)
    , d0_(NAN), d4_(NAN), s0_(NAN), s4_(NAN), j0_(NAN)
    {}

    void SplineC2aS4
                ::setXs(double a_x0, double a_x1, double a_x2, double a_x3, double a_x4)
    {
        h0_ = a_x1 - a_x0;
        h1_ = a_x2 - a_x1;
        h2_ = a_x3 - a_x2;
        h3_ = a_x4 - a_x3;
    }

    void SplineC2aS4
                ::setYs(double a_y0, double a_y1, double a_y2, double a_y3, double a_y4)
    {
        y0_ = a_y0;
        y1_ = a_y1;
        y2_ = a_y2;
        y3_ = a_y3;
        y4_ = a_y4;
#ifdef DEBUG_PRINT_SPLINE_COMPUTATION
        std::cout << "Spline computation: " << y0_ << " ~ " << y1_ << " ~ " << y2_ << " ~ " << y3_ << " ~ " << y4_ << std::endl;
#endif
    }

    void SplineC2aS4
                ::setStartConds(double a_d0, double a_s0, double a_j0)
    {
        d0_ = a_d0;
        s0_ = a_s0;
        j0_ = a_j0;
    }

    void SplineC2aS4
                ::setEndConds(double a_d4, double a_s4)
    {
        d4_ = a_d4;
        s4_ = a_s4;
    }

    void SplineC2aS4
                ::computeAndSaveTo(std::vector<Degree6Polynomial>& results)
    {
        s3e_.clear();
        s3e_.addEquation1(s3p0_.valueAt(0.), y0_);
        s3e_.addEquation1(s3p0_.valueAt(h0_), y1_);
        s3e_.addEquation1(s3p0_.derivativeAt(0.), d0_);
        s3e_.addEquation2(s3p0_.derivativeAt(h0_), s3p1_.derivativeAt(0.));
        s3e_.addEquation1(s3p0_.secondDerivativeAt(0.), s0_);
        s3e_.addEquation2(s3p0_.secondDerivativeAt(h0_), s3p1_.secondDerivativeAt(0.));
        s3e_.addEquation1(s3p0_.thirdDerivativeAt(0.), j0_);
        s3e_.addEquation1(s3p1_.valueAt(0.), y1_);
        s3e_.addEquation1(s3p1_.valueAt(h1_), y2_);
        s3e_.addEquation2(s3p1_.derivativeAt(h1_), s3p2_.derivativeAt(0.));
        s3e_.addEquation2(s3p1_.secondDerivativeAt(h1_), s3p2_.secondDerivativeAt(0.));
        s3e_.addEquation1(s3p2_.valueAt(0.), y2_);
        s3e_.addEquation1(s3p2_.valueAt(h2_), y3_);
        s3e_.addEquation2(s3p2_.derivativeAt(h2_), s3p3_.derivativeAt(0.));
        s3e_.addEquation2(s3p2_.secondDerivativeAt(h2_), s3p3_.secondDerivativeAt(0.));
        s3e_.addEquation1(s3p3_.valueAt(0.), y3_);
        s3e_.addEquation1(s3p3_.valueAt(h3_), y4_);
        s3e_.addEquation1(s3p3_.derivativeAt(h3_), d4_);
        s3e_.addEquation1(s3p3_.secondDerivativeAt(h3_), s4_);
        s3e_.solve();
        results.push_back(s3e_.buildDegree6Polynomial(s3p0_));
        results.push_back(s3e_.buildDegree6Polynomial(s3p1_));
        results.push_back(s3e_.buildDegree6Polynomial(s3p2_));
        results.push_back(s3e_.buildDegree6Polynomial(s3p3_));
    }

///////////////////////////////////////////////////////////////////////////////
// class SplineC2aS3                                                         //
///////////////////////////////////////////////////////////////////////////////

    SplineC2aS3
                ::SplineC2aS3()
    : s3c_()
    , s3p0_(s3c_.addPolynomial(fnGetEnvIntWithDefault("ST_C2S3_D0", 5)))
    , s3p1_(s3c_.addPolynomial(fnGetEnvIntWithDefault("ST_C2S3_D1", 3)))
    , s3p2_(s3c_.addPolynomial(fnGetEnvIntWithDefault("ST_C2S3_D2", 4)))
    , s3e_(s3c_)
    , y0_(NAN), y1_(NAN), y2_(NAN), y3_(NAN)
    , h0_(NAN), h1_(NAN), h2_(NAN)
    , d0_(NAN), d3_(NAN), s0_(NAN), s3_(NAN), j0_(NAN)
    {}

    void SplineC2aS3
                ::setXs(double a_x0, double a_x1, double a_x2, double a_x3)
    {
        h0_ = a_x1 - a_x0;
        h1_ = a_x2 - a_x1;
        h2_ = a_x3 - a_x2;
    }

    void SplineC2aS3
                ::setYs(double a_y0, double a_y1, double a_y2, double a_y3)
    {
        y0_ = a_y0;
        y1_ = a_y1;
        y2_ = a_y2;
        y3_ = a_y3;
#ifdef DEBUG_PRINT_SPLINE_COMPUTATION
        std::cout << "Spline computation: " << y0_ << " ~ " << y1_ << " ~ " << y2_ << " ~ " << y3_ << std::endl;
#endif
    }

    void SplineC2aS3
                ::setStartConds(double a_d0, double a_s0, double a_j0)
    {
        d0_ = a_d0;
        s0_ = a_s0;
        j0_ = a_j0;
    }

    void SplineC2aS3
                ::setEndConds(double a_d3, double a_s3)
    {
        d3_ = a_d3;
        s3_ = a_s3;
    }

    void SplineC2aS3
                ::computeAndSaveTo(std::vector<Degree6Polynomial>& results)
    {
        s3e_.clear();
        s3e_.addEquation1(s3p0_.valueAt(0.), y0_);
        s3e_.addEquation1(s3p0_.valueAt(h0_), y1_);
        s3e_.addEquation1(s3p0_.derivativeAt(0.), d0_);
        s3e_.addEquation2(s3p0_.derivativeAt(h0_), s3p1_.derivativeAt(0.));
        s3e_.addEquation1(s3p0_.secondDerivativeAt(0.), s0_);
        s3e_.addEquation2(s3p0_.secondDerivativeAt(h0_), s3p1_.secondDerivativeAt(0.));
        s3e_.addEquation1(s3p0_.thirdDerivativeAt(0.), j0_);
        s3e_.addEquation1(s3p1_.valueAt(0.), y1_);
        s3e_.addEquation1(s3p1_.valueAt(h1_), y2_);
        s3e_.addEquation2(s3p1_.derivativeAt(h1_), s3p2_.derivativeAt(0.));
        s3e_.addEquation2(s3p1_.secondDerivativeAt(h1_), s3p2_.secondDerivativeAt(0.));
        s3e_.addEquation1(s3p2_.valueAt(0.), y2_);
        s3e_.addEquation1(s3p2_.valueAt(h2_), y3_);
        s3e_.addEquation1(s3p2_.derivativeAt(h2_), d3_);
        s3e_.addEquation1(s3p2_.secondDerivativeAt(h2_), s3_);
        s3e_.solve();
        results.push_back(s3e_.buildDegree6Polynomial(s3p0_));
        results.push_back(s3e_.buildDegree6Polynomial(s3p1_));
        results.push_back(s3e_.buildDegree6Polynomial(s3p2_));
    }

///////////////////////////////////////////////////////////////////////////////
// class SplineC2aS2                                                         //
///////////////////////////////////////////////////////////////////////////////

    SplineC2aS2
                ::SplineC2aS2()
    : s2c_()
    , s2p0_(s2c_.addPolynomial(fnGetEnvIntWithDefault("ST_C2aS2_D0", 5)))
    , s2p1_(s2c_.addPolynomial(fnGetEnvIntWithDefault("ST_C2aS2_D1", 4)))
    , s2e_(s2c_)
    , y0_(NAN), y1_(NAN), y2_(NAN), h0_(NAN), h1_(NAN), d0_(NAN), d2_(NAN), s0_(NAN), s2_(NAN), j0_(NAN)
    {}

    void SplineC2aS2
                ::setXs(double a_x0, double a_x1, double a_x2)
    {
        h0_ = a_x1 - a_x0;
        h1_ = a_x2 - a_x1;
    }

    void SplineC2aS2
                ::setYs(double a_y0, double a_y1, double a_y2)
    {
        y0_ = a_y0;
        y1_ = a_y1;
        y2_ = a_y2;
#ifdef DEBUG_PRINT_SPLINE_COMPUTATION
        std::cout << "Spline computation: " << y0_ << " ~ " << y1_ << " ~ " << y2_ << std::endl;
#endif
    }

    void SplineC2aS2
                ::setStartConds(double a_d0, double a_s0, double a_j0)
    {
        d0_ = a_d0;
        s0_ = a_s0;
        j0_ = a_j0;
    }

    void SplineC2aS2
                ::setEndConds(double a_d2, double a_s2)
    {
        d2_ = a_d2;
        s2_ = a_s2;
    }

    void SplineC2aS2
                ::computeAndSaveTo(std::vector<Degree6Polynomial>& results)
    {
        s2e_.clear();
        s2e_.addEquation1(s2p0_.valueAt(0.), y0_);
        s2e_.addEquation1(s2p0_.valueAt(h0_), y1_);
        s2e_.addEquation1(s2p0_.derivativeAt(0.), d0_);
        s2e_.addEquation2(s2p0_.derivativeAt(h0_), s2p1_.derivativeAt(0.));
        s2e_.addEquation1(s2p0_.secondDerivativeAt(0.), s0_);
        s2e_.addEquation2(s2p0_.secondDerivativeAt(h0_), s2p1_.secondDerivativeAt(0.));
        s2e_.addEquation1(s2p0_.thirdDerivativeAt(0.), j0_);
        s2e_.addEquation1(s2p1_.valueAt(0.), y1_);
        s2e_.addEquation1(s2p1_.valueAt(h1_), y2_);
        s2e_.addEquation1(s2p1_.derivativeAt(h1_), d2_);
        s2e_.addEquation1(s2p1_.secondDerivativeAt(h1_), s2_);
        s2e_.solve();
        results.push_back(s2e_.buildDegree6Polynomial(s2p0_));
        results.push_back(s2e_.buildDegree6Polynomial(s2p1_));
    }

///////////////////////////////////////////////////////////////////////////////
// class SplineC2aS1                                                         //
///////////////////////////////////////////////////////////////////////////////

    SplineC2aS1
                ::SplineC2aS1()
    : s1c_()
    , s1p0_(s1c_.addPolynomial(fnGetEnvIntWithDefault("ST_C2aS1_D0", 6)))
    , s1e_(s1c_)
    , y0_(NAN), y1_(NAN), h0_(NAN), d0_(NAN), d1_(NAN), s0_(NAN), s1_(NAN), j0_(NAN)
    {}

    void SplineC2aS1
                ::setXs(double a_x0, double a_x1)
    {
        h0_ = a_x1 - a_x0;
    }

    void SplineC2aS1
                ::setYs(double a_y0, double a_y1)
    {
        y0_ = a_y0;
        y1_ = a_y1;
#ifdef DEBUG_PRINT_SPLINE_COMPUTATION
        std::cout << "Spline computation: " << y0_ << " ~ " << y1_ << std::endl;
#endif
    }

    void SplineC2aS1
                ::setStartConds(double a_d0, double a_s0, double a_j0)
    {
        d0_ = a_d0;
        s0_ = a_s0;
        j0_ = a_j0;
    }

    void SplineC2aS1
                ::setEndConds(double a_d1, double a_s1)
    {
        d1_ = a_d1;
        s1_ = a_s1;
    }

    void SplineC2aS1
                ::computeAndSaveTo(std::vector<Degree6Polynomial>& results)
    {
        s1e_.clear();
        s1e_.addEquation1(s1p0_.valueAt(0.), y0_);
        s1e_.addEquation1(s1p0_.valueAt(h0_), y1_);
        s1e_.addEquation1(s1p0_.derivativeAt(0.), d0_);
        s1e_.addEquation1(s1p0_.derivativeAt(h0_), d1_);
        s1e_.addEquation1(s1p0_.secondDerivativeAt(0.), s0_);
        s1e_.addEquation1(s1p0_.secondDerivativeAt(h0_), s1_);
        s1e_.addEquation1(s1p0_.thirdDerivativeAt(0.), j0_);
        s1e_.solve();
        results.push_back(s1e_.buildDegree6Polynomial(s1p0_));
    }

///////////////////////////////////////////////////////////////////////////////
// class SplineC3S5                                                          //
///////////////////////////////////////////////////////////////////////////////

    SplineC3S5
                ::SplineC3S5()
    : s3c_()
    , s3p0_(s3c_.addPolynomial(fnGetEnvIntWithDefault("ST_C3S5_D0", 6)))
    , s3p1_(s3c_.addPolynomial(fnGetEnvIntWithDefault("ST_C3S5_D1", 4)))
    , s3p2_(s3c_.addPolynomial(fnGetEnvIntWithDefault("ST_C3S5_D2", 3)))
    , s3p3_(s3c_.addPolynomial(fnGetEnvIntWithDefault("ST_C3S5_D3", 4)))
    , s3p4_(s3c_.addPolynomial(fnGetEnvIntWithDefault("ST_C3S5_D4", 6)))
    , s3e_(s3c_)
    , y0_(NAN), y1_(NAN), y2_(NAN), y3_(NAN), y4_(NAN), y5_(NAN)
    , h0_(NAN), h1_(NAN), h2_(NAN), h3_(NAN), h4_(NAN)
    , d0_(NAN), d5_(NAN), s0_(NAN), s5_(NAN), j0_(NAN), j5_(NAN)
    {}

    void SplineC3S5
                ::setXs(double a_x0, double a_x1, double a_x2, double a_x3, double a_x4, double a_x5)
    {
        h0_ = a_x1 - a_x0;
        h1_ = a_x2 - a_x1;
        h2_ = a_x3 - a_x2;
        h3_ = a_x4 - a_x3;
        h4_ = a_x5 - a_x4;
    }
    
    void SplineC3S5
                ::setYs(double a_y0, double a_y1, double a_y2, double a_y3, double a_y4, double a_y5)
    {
        y0_ = a_y0;
        y1_ = a_y1;
        y2_ = a_y2;
        y3_ = a_y3;
        y4_ = a_y4;
        y5_ = a_y5;
#ifdef DEBUG_PRINT_SPLINE_COMPUTATION
        std::cout << "Spline computation: " << y0_ << " ~ " << y1_ << " ~ " << y2_ << " ~ " << y3_ << " ~ " << y4_ << " ~ " << y5_ << std::endl;
#endif
    }
    
    void SplineC3S5
                ::setStartConds(double a_d0, double a_s0, double a_j0)
    {
        d0_ = a_d0;
        s0_ = a_s0;
        j0_ = a_j0;
    }
    
    void SplineC3S5
                ::setEndConds(double a_d5, double a_s5, double a_j5)
    {
        d5_ = a_d5;
        s5_ = a_s5;
        j5_ = a_j5;
    }
    
    void SplineC3S5
                ::computeAndSaveTo(std::vector<Degree7Polynomial>& results)
    {
        s3e_.clear();
        s3e_.addEquation1(s3p0_.valueAt(0.), y0_);
        s3e_.addEquation1(s3p0_.valueAt(h0_), y1_);
        s3e_.addEquation1(s3p0_.derivativeAt(0.), d0_);
        s3e_.addEquation2(s3p0_.derivativeAt(h0_), s3p1_.derivativeAt(0.));
        s3e_.addEquation1(s3p0_.secondDerivativeAt(0.), s0_);
        s3e_.addEquation1(s3p0_.thirdDerivativeAt(0.), j0_);
        s3e_.addEquation2(s3p0_.secondDerivativeAt(h0_), s3p1_.secondDerivativeAt(0.));
        s3e_.addEquation2(s3p0_.thirdDerivativeAt(h0_), s3p1_.thirdDerivativeAt(0.));
        s3e_.addEquation1(s3p1_.valueAt(0.), y1_);
        s3e_.addEquation1(s3p1_.valueAt(h1_), y2_);
        s3e_.addEquation2(s3p1_.derivativeAt(h1_), s3p2_.derivativeAt(0.));
        s3e_.addEquation2(s3p1_.secondDerivativeAt(h1_), s3p2_.secondDerivativeAt(0.));
        s3e_.addEquation2(s3p1_.thirdDerivativeAt(h1_), s3p2_.thirdDerivativeAt(0.));
        s3e_.addEquation1(s3p2_.valueAt(0.), y2_);
        s3e_.addEquation1(s3p2_.valueAt(h2_), y3_);
        s3e_.addEquation2(s3p2_.derivativeAt(h2_), s3p3_.derivativeAt(0.));
        s3e_.addEquation2(s3p2_.secondDerivativeAt(h2_), s3p3_.secondDerivativeAt(0.));
        s3e_.addEquation2(s3p2_.thirdDerivativeAt(h2_), s3p3_.thirdDerivativeAt(0.));
        s3e_.addEquation1(s3p3_.valueAt(0.), y3_);
        s3e_.addEquation1(s3p3_.valueAt(h3_), y4_);
        s3e_.addEquation2(s3p3_.derivativeAt(h3_), s3p4_.derivativeAt(0.));
        s3e_.addEquation2(s3p3_.secondDerivativeAt(h3_), s3p4_.secondDerivativeAt(0.));
        s3e_.addEquation2(s3p3_.thirdDerivativeAt(h3_), s3p4_.thirdDerivativeAt(0.));
        s3e_.addEquation1(s3p4_.valueAt(0.), y4_);
        s3e_.addEquation1(s3p4_.valueAt(h4_), y5_);
        s3e_.addEquation1(s3p4_.derivativeAt(h4_), d5_);
        s3e_.addEquation1(s3p4_.secondDerivativeAt(h4_), s5_);
        s3e_.addEquation1(s3p4_.thirdDerivativeAt(h4_), j5_);
        s3e_.solve();
        results.push_back(s3e_.buildDegree7Polynomial(s3p0_));
        results.push_back(s3e_.buildDegree7Polynomial(s3p1_));
        results.push_back(s3e_.buildDegree7Polynomial(s3p2_));
        results.push_back(s3e_.buildDegree7Polynomial(s3p3_));
        results.push_back(s3e_.buildDegree7Polynomial(s3p4_));
    }

///////////////////////////////////////////////////////////////////////////////
// class SplineC3S4                                                          //
///////////////////////////////////////////////////////////////////////////////

    SplineC3S4
                ::SplineC3S4()
    : s3c_()
    , s3p0_(s3c_.addPolynomial(fnGetEnvIntWithDefault("ST_C3S4_D0", 6)))
    , s3p1_(s3c_.addPolynomial(fnGetEnvIntWithDefault("ST_C3S4_D1", 4)))
    , s3p2_(s3c_.addPolynomial(fnGetEnvIntWithDefault("ST_C3S4_D2", 3)))
    , s3p3_(s3c_.addPolynomial(fnGetEnvIntWithDefault("ST_C3S4_D3", 6)))
    , s3e_(s3c_)
    , y0_(NAN), y1_(NAN), y2_(NAN), y3_(NAN), y4_(NAN)
    , h0_(NAN), h1_(NAN), h2_(NAN), h3_(NAN)
    , d0_(NAN), d4_(NAN), s0_(NAN), s4_(NAN), j0_(NAN), j4_(NAN)
    {}

    void SplineC3S4
                ::setXs(double a_x0, double a_x1, double a_x2, double a_x3, double a_x4)
    {
        h0_ = a_x1 - a_x0;
        h1_ = a_x2 - a_x1;
        h2_ = a_x3 - a_x2;
        h3_ = a_x4 - a_x3;
    }
    
    void SplineC3S4
                ::setYs(double a_y0, double a_y1, double a_y2, double a_y3, double a_y4)
    {
        y0_ = a_y0;
        y1_ = a_y1;
        y2_ = a_y2;
        y3_ = a_y3;
        y4_ = a_y4;
#ifdef DEBUG_PRINT_SPLINE_COMPUTATION
        std::cout << "Spline computation: " << y0_ << " ~ " << y1_ << " ~ " << y2_ << " ~ " << y3_ << " ~ " << y4_ << std::endl;
#endif
    }
    
    void SplineC3S4
                ::setStartConds(double a_d0, double a_s0, double a_j0)
    {
        d0_ = a_d0;
        s0_ = a_s0;
        j0_ = a_j0;
    }
    
    void SplineC3S4
                ::setEndConds(double a_d4, double a_s4, double a_j4)
    {
        d4_ = a_d4;
        s4_ = a_s4;
        j4_ = a_j4;
    }
    
    void SplineC3S4
                ::computeAndSaveTo(std::vector<Degree7Polynomial>& results)
    {
        s3e_.clear();
        s3e_.addEquation1(s3p0_.valueAt(0.), y0_);
        s3e_.addEquation1(s3p0_.valueAt(h0_), y1_);
        s3e_.addEquation1(s3p0_.derivativeAt(0.), d0_);
        s3e_.addEquation2(s3p0_.derivativeAt(h0_), s3p1_.derivativeAt(0.));
        s3e_.addEquation1(s3p0_.secondDerivativeAt(0.), s0_);
        s3e_.addEquation1(s3p0_.thirdDerivativeAt(0.), j0_);
        s3e_.addEquation2(s3p0_.secondDerivativeAt(h0_), s3p1_.secondDerivativeAt(0.));
        s3e_.addEquation2(s3p0_.thirdDerivativeAt(h0_), s3p1_.thirdDerivativeAt(0.));
        s3e_.addEquation1(s3p1_.valueAt(0.), y1_);
        s3e_.addEquation1(s3p1_.valueAt(h1_), y2_);
        s3e_.addEquation2(s3p1_.derivativeAt(h1_), s3p2_.derivativeAt(0.));
        s3e_.addEquation2(s3p1_.secondDerivativeAt(h1_), s3p2_.secondDerivativeAt(0.));
        s3e_.addEquation2(s3p1_.thirdDerivativeAt(h1_), s3p2_.thirdDerivativeAt(0.));
        s3e_.addEquation1(s3p2_.valueAt(0.), y2_);
        s3e_.addEquation1(s3p2_.valueAt(h2_), y3_);
        s3e_.addEquation2(s3p2_.derivativeAt(h2_), s3p3_.derivativeAt(0.));
        s3e_.addEquation2(s3p2_.secondDerivativeAt(h2_), s3p3_.secondDerivativeAt(0.));
        s3e_.addEquation2(s3p2_.thirdDerivativeAt(h2_), s3p3_.thirdDerivativeAt(0.));
        s3e_.addEquation1(s3p3_.valueAt(0.), y3_);
        s3e_.addEquation1(s3p3_.valueAt(h3_), y4_);
        s3e_.addEquation1(s3p3_.derivativeAt(h3_), d4_);
        s3e_.addEquation1(s3p3_.secondDerivativeAt(h3_), s4_);
        s3e_.addEquation1(s3p3_.thirdDerivativeAt(h3_), j4_);
        s3e_.solve();
        results.push_back(s3e_.buildDegree7Polynomial(s3p0_));
        results.push_back(s3e_.buildDegree7Polynomial(s3p1_));
        results.push_back(s3e_.buildDegree7Polynomial(s3p2_));
        results.push_back(s3e_.buildDegree7Polynomial(s3p3_));
    }

///////////////////////////////////////////////////////////////////////////////
// class SplineC3S3                                                          //
///////////////////////////////////////////////////////////////////////////////

    SplineC3S3
                ::SplineC3S3()
    : s3c_()
    , s3p0_(s3c_.addPolynomial(fnGetEnvIntWithDefault("ST_C3S3_D0", 6)))
    , s3p1_(s3c_.addPolynomial(fnGetEnvIntWithDefault("ST_C3S3_D1", 3)))
    , s3p2_(s3c_.addPolynomial(fnGetEnvIntWithDefault("ST_C3S3_D2", 6)))
    , s3e_(s3c_)
    , y0_(NAN), y1_(NAN), y2_(NAN), y3_(NAN)
    , h0_(NAN), h1_(NAN), h2_(NAN)
    , d0_(NAN), d3_(NAN), s0_(NAN), s3_(NAN), j0_(NAN), j3_(NAN)
    {}

    void SplineC3S3
                ::setXs(double a_x0, double a_x1, double a_x2, double a_x3)
    {
        h0_ = a_x1 - a_x0;
        h1_ = a_x2 - a_x1;
        h2_ = a_x3 - a_x2;
    }

    void SplineC3S3
                ::setYs(double a_y0, double a_y1, double a_y2, double a_y3)
    {
        y0_ = a_y0;
        y1_ = a_y1;
        y2_ = a_y2;
        y3_ = a_y3;
#ifdef DEBUG_PRINT_SPLINE_COMPUTATION
        std::cout << "Spline computation: " << y0_ << " ~ " << y1_ << " ~ " << y2_ << " ~ " << y3_ << std::endl;
#endif
    }

    void SplineC3S3
                ::setStartConds(double a_d0, double a_s0, double a_j0)
    {
        d0_ = a_d0;
        s0_ = a_s0;
        j0_ = a_j0;
    }

    void SplineC3S3
                ::setEndConds(double a_d3, double a_s3, double a_j3)
    {
        d3_ = a_d3;
        s3_ = a_s3;
        j3_ = a_j3;
    }

    void SplineC3S3
                ::computeAndSaveTo(std::vector<Degree7Polynomial>& results)
    {
#ifdef DEBUG_PRINT_SPLINE_COMPUTATION
        std::cout << "(" << y0_ << " ~ " << d0_ << " ~ " << s0_ << " ~ " << j0_ << ") " << h0_
                 << " (" << y1_ << ") " << h1_
                 << " (" << y2_ << ") " << h2_
                 << " (" << y3_ << " ~ " << d3_ << " ~ " << s3_ << " ~ " << j3_ << ")" << std::endl;
#endif
        s3e_.clear();
        s3e_.addEquation1(s3p0_.valueAt(0.), y0_);
        s3e_.addEquation1(s3p0_.valueAt(h0_), y1_);
        s3e_.addEquation1(s3p0_.derivativeAt(0.), d0_);
        s3e_.addEquation2(s3p0_.derivativeAt(h0_), s3p1_.derivativeAt(0.));
        s3e_.addEquation1(s3p0_.secondDerivativeAt(0.), s0_);
        s3e_.addEquation1(s3p0_.thirdDerivativeAt(0.), j0_);
        s3e_.addEquation2(s3p0_.secondDerivativeAt(h0_), s3p1_.secondDerivativeAt(0.));
        s3e_.addEquation2(s3p0_.thirdDerivativeAt(h0_), s3p1_.thirdDerivativeAt(0.));
        s3e_.addEquation1(s3p1_.valueAt(0.), y1_);
        s3e_.addEquation1(s3p1_.valueAt(h1_), y2_);
        s3e_.addEquation2(s3p1_.derivativeAt(h1_), s3p2_.derivativeAt(0.));
        s3e_.addEquation2(s3p1_.secondDerivativeAt(h1_), s3p2_.secondDerivativeAt(0.));
        s3e_.addEquation2(s3p1_.thirdDerivativeAt(h1_), s3p2_.thirdDerivativeAt(0.));
        s3e_.addEquation1(s3p2_.valueAt(0.), y2_);
        s3e_.addEquation1(s3p2_.valueAt(h2_), y3_);
        s3e_.addEquation1(s3p2_.derivativeAt(h2_), d3_);
        s3e_.addEquation1(s3p2_.secondDerivativeAt(h2_), s3_);
        s3e_.addEquation1(s3p2_.thirdDerivativeAt(h2_), j3_);
        s3e_.solve();
        results.push_back(s3e_.buildDegree7Polynomial(s3p0_));
        results.push_back(s3e_.buildDegree7Polynomial(s3p1_));
        results.push_back(s3e_.buildDegree7Polynomial(s3p2_));
    }

///////////////////////////////////////////////////////////////////////////////
// class SplineC3S2                                                          //
///////////////////////////////////////////////////////////////////////////////

    SplineC3S2
                ::SplineC3S2()
    : s2c_()
    , s2p0_(s2c_.addPolynomial(fnGetEnvIntWithDefault("ST_C3S2_D0", 6)))
    , s2p1_(s2c_.addPolynomial(fnGetEnvIntWithDefault("ST_C3S2_D1", 5)))
    , s2e_(s2c_)
    , y0_(NAN), y1_(NAN), y2_(NAN), h0_(NAN), h1_(NAN), d0_(NAN), d2_(NAN), s0_(NAN), s2_(NAN), j0_(NAN), j2_(NAN)
    {}

    void SplineC3S2
                ::setXs(double a_x0, double a_x1, double a_x2)
    {
        h0_ = a_x1 - a_x0;
        h1_ = a_x2 - a_x1;
    }

    void SplineC3S2
                ::setYs(double a_y0, double a_y1, double a_y2)
    {
        y0_ = a_y0;
        y1_ = a_y1;
        y2_ = a_y2;
#ifdef DEBUG_PRINT_SPLINE_COMPUTATION
        std::cout << "Spline computation: " << y0_ << " ~ " << y1_ << " ~ " << y2_ << std::endl;
#endif
    }

    void SplineC3S2
                ::setStartConds(double a_d0, double a_s0, double a_j0)
    {
        d0_ = a_d0;
        s0_ = a_s0;
        j0_ = a_j0;
    }

    void SplineC3S2
                ::setEndConds(double a_d2, double a_s2, double a_j2)
    {
        d2_ = a_d2;
        s2_ = a_s2;
        j2_ = a_j2;
    }

    void SplineC3S2
                ::computeAndSaveTo(std::vector<Degree7Polynomial>& results)
    {
        s2e_.clear();
        s2e_.addEquation1(s2p0_.valueAt(0.), y0_);
        s2e_.addEquation1(s2p0_.valueAt(h0_), y1_);
        s2e_.addEquation1(s2p0_.derivativeAt(0.), d0_);
        s2e_.addEquation2(s2p0_.derivativeAt(h0_), s2p1_.derivativeAt(0.));
        s2e_.addEquation1(s2p0_.secondDerivativeAt(0.), s0_);
        s2e_.addEquation2(s2p0_.secondDerivativeAt(h0_), s2p1_.secondDerivativeAt(0.));
        s2e_.addEquation1(s2p0_.thirdDerivativeAt(0.), j0_);
        s2e_.addEquation2(s2p0_.thirdDerivativeAt(h0_), s2p1_.thirdDerivativeAt(0.));
        s2e_.addEquation1(s2p1_.valueAt(0.), y1_);
        s2e_.addEquation1(s2p1_.valueAt(h1_), y2_);
        s2e_.addEquation1(s2p1_.derivativeAt(h1_), d2_);
        s2e_.addEquation1(s2p1_.secondDerivativeAt(h1_), s2_);
        s2e_.addEquation1(s2p1_.thirdDerivativeAt(h1_), j2_);
        s2e_.solve();
        results.push_back(s2e_.buildDegree7Polynomial(s2p0_));
        results.push_back(s2e_.buildDegree7Polynomial(s2p1_));
    }

///////////////////////////////////////////////////////////////////////////////
// class SplineC3S1                                                          //
///////////////////////////////////////////////////////////////////////////////

    SplineC3S1
                ::SplineC3S1()
    : s1c_()
    , s1p0_(s1c_.addPolynomial(fnGetEnvIntWithDefault("ST_C3S1_D0", 7)))
    , s1e_(s1c_)
    , y0_(NAN), y1_(NAN), h0_(NAN), d0_(NAN), d1_(NAN), s0_(NAN), s1_(NAN), j0_(NAN), j1_(NAN)
    {}

    void SplineC3S1
                ::setXs(double a_x0, double a_x1)
    {
        h0_ = a_x1 - a_x0;
    }

    void SplineC3S1
                ::setYs(double a_y0, double a_y1)
    {
        y0_ = a_y0;
        y1_ = a_y1;
#ifdef DEBUG_PRINT_SPLINE_COMPUTATION
        std::cout << "Spline computation: " << y0_ << " ~ " << y1_ << std::endl;
#endif
    }

    void SplineC3S1
                ::setStartConds(double a_d0, double a_s0, double a_j0)
    {
        d0_ = a_d0;
        s0_ = a_s0;
        j0_ = a_j0;
    }

    void SplineC3S1
                ::setEndConds(double a_d1, double a_s1, double a_j1)
    {
        d1_ = a_d1;
        s1_ = a_s1;
        j1_ = a_j1;
    }

    void SplineC3S1
                ::computeAndSaveTo(std::vector<Degree7Polynomial>& results)
    {
        s1e_.clear();
        s1e_.addEquation1(s1p0_.valueAt(0.), y0_);
        s1e_.addEquation1(s1p0_.valueAt(h0_), y1_);
        s1e_.addEquation1(s1p0_.derivativeAt(0.), d0_);
        s1e_.addEquation1(s1p0_.derivativeAt(h0_), d1_);
        s1e_.addEquation1(s1p0_.secondDerivativeAt(0.), s0_);
        s1e_.addEquation1(s1p0_.secondDerivativeAt(h0_), s1_);
        s1e_.addEquation1(s1p0_.thirdDerivativeAt(0.), j0_);
        s1e_.addEquation1(s1p0_.thirdDerivativeAt(h0_), j1_);
        s1e_.solve();
        results.push_back(s1e_.buildDegree7Polynomial(s1p0_));
    }

///////////////////////////////////////////////////////////////////////////////
// class SplineC3xS4                                                         //
///////////////////////////////////////////////////////////////////////////////

    SplineC3xS4
                ::SplineC3xS4()
    : s3c_()
    , s3p0_(s3c_.addPolynomial(fnGetEnvIntWithDefault("ST_C3xS4_D0", 6)))
    , s3p1_(s3c_.addPolynomial(fnGetEnvIntWithDefault("ST_C3xS4_D1", 3)))
    , s3p2_(s3c_.addPolynomial(fnGetEnvIntWithDefault("ST_C3xS4_D2", 3)))
    , s3p3_(s3c_.addPolynomial(fnGetEnvIntWithDefault("ST_C3xS4_D3", 6)))
    , s3e_(s3c_)
    , y0_(NAN), y1_(NAN), y2_(NAN), y3_(NAN), y4_(NAN)
    , h0_(NAN), h1_(NAN), h2_(NAN), h3_(NAN)
    , d0_(NAN), d3_(NAN), s0_(NAN), s3_(NAN), j0_(NAN)
    {}

    void SplineC3xS4
                ::setXs(double a_x0, double a_x1, double a_x2, double a_x3, double a_x4)
    {
        h0_ = a_x1 - a_x0;
        h1_ = a_x2 - a_x1;
        h2_ = a_x3 - a_x2;
        h3_ = a_x4 - a_x3;
    }

    void SplineC3xS4
                ::setYs(double a_y0, double a_y1, double a_y2, double a_y3, double a_y4)
    {
        y0_ = a_y0;
        y1_ = a_y1;
        y2_ = a_y2;
        y3_ = a_y3;
        y4_ = a_y4;
#ifdef DEBUG_PRINT_SPLINE_COMPUTATION
        std::cout << "Spline computation: " << y0_ << " ~ " << y1_ << " ~ " << y2_ << " ~ " << y3_ << " ~ " << y4_ << std::endl;
#endif
    }

    void SplineC3xS4
                ::setStartConds(double a_d0, double a_s0, double a_j0)
    {
        d0_ = a_d0;
        s0_ = a_s0;
        j0_ = a_j0;
    }

    void SplineC3xS4
                ::setEndConds(double a_d3, double a_s3)
    {
        d3_ = a_d3;
        s3_ = a_s3;
    }

    void SplineC3xS4
                ::computeAndSaveTo(std::vector<Degree6Polynomial>& results)
    {
        s3e_.clear();
        s3e_.addEquation1(s3p0_.valueAt(0.), y0_);
        s3e_.addEquation1(s3p0_.valueAt(h0_), y1_);
        s3e_.addEquation1(s3p0_.derivativeAt(0.), d0_);
        s3e_.addEquation2(s3p0_.derivativeAt(h0_), s3p1_.derivativeAt(0.));
        s3e_.addEquation1(s3p0_.secondDerivativeAt(0.), s0_);
        s3e_.addEquation1(s3p0_.thirdDerivativeAt(0.), j0_);
        s3e_.addEquation2(s3p0_.secondDerivativeAt(h0_), s3p1_.secondDerivativeAt(0.));
        s3e_.addEquation2(s3p0_.thirdDerivativeAt(h0_), s3p1_.thirdDerivativeAt(0.));
        s3e_.addEquation1(s3p1_.valueAt(0.), y1_);
        s3e_.addEquation1(s3p1_.valueAt(h1_), y2_);
        s3e_.addEquation2(s3p1_.derivativeAt(h1_), s3p2_.derivativeAt(0.));
        s3e_.addEquation2(s3p1_.secondDerivativeAt(h1_), s3p2_.secondDerivativeAt(0.));
        s3e_.addEquation2(s3p1_.thirdDerivativeAt(h1_), s3p2_.thirdDerivativeAt(0.));
        s3e_.addEquation1(s3p2_.valueAt(0.), y2_);
        s3e_.addEquation1(s3p2_.valueAt(h2_), y3_);
        s3e_.addEquation2(s3p2_.derivativeAt(h2_), s3p3_.derivativeAt(0.));
        s3e_.addEquation2(s3p2_.secondDerivativeAt(h2_), s3p3_.secondDerivativeAt(0.));
        s3e_.addEquation2(s3p2_.thirdDerivativeAt(h2_), s3p3_.thirdDerivativeAt(0.));
        s3e_.addEquation1(s3p3_.valueAt(0.), y3_);
        s3e_.addEquation1(s3p3_.valueAt(h3_), y4_);
        s3e_.addEquation1(s3p3_.derivativeAt(h2_), d3_);
        s3e_.addEquation1(s3p3_.secondDerivativeAt(h2_), s3_);
        s3e_.solve();
        results.push_back(s3e_.buildDegree6Polynomial(s3p0_));
        results.push_back(s3e_.buildDegree6Polynomial(s3p1_));
        results.push_back(s3e_.buildDegree6Polynomial(s3p2_));
    }

///////////////////////////////////////////////////////////////////////////////
// class SplineC3xS3                                                         //
///////////////////////////////////////////////////////////////////////////////

    SplineC3xS3
                ::SplineC3xS3()
    : s3c_()
    , s3p0_(s3c_.addPolynomial(fnGetEnvIntWithDefault("ST_C3xS3_D0", 6)))
    , s3p1_(s3c_.addPolynomial(fnGetEnvIntWithDefault("ST_C3xS3_D1", 2)))
    , s3p2_(s3c_.addPolynomial(fnGetEnvIntWithDefault("ST_C3xS3_D2", 6)))
    , s3e_(s3c_)
    , y0_(NAN), y1_(NAN), y2_(NAN), y3_(NAN)
    , h0_(NAN), h1_(NAN), h2_(NAN)
    , d0_(NAN), d3_(NAN), s0_(NAN), s3_(NAN), j0_(NAN)
    {}

    void SplineC3xS3
                ::setXs(double a_x0, double a_x1, double a_x2, double a_x3)
    {
        h0_ = a_x1 - a_x0;
        h1_ = a_x2 - a_x1;
        h2_ = a_x3 - a_x2;
    }

    void SplineC3xS3
                ::setYs(double a_y0, double a_y1, double a_y2, double a_y3)
    {
        y0_ = a_y0;
        y1_ = a_y1;
        y2_ = a_y2;
        y3_ = a_y3;
#ifdef DEBUG_PRINT_SPLINE_COMPUTATION
        std::cout << "Spline computation: " << y0_ << " ~ " << y1_ << " ~ " << y2_ << " ~ " << y3_ << std::endl;
#endif
    }

    void SplineC3xS3
                ::setStartConds(double a_d0, double a_s0, double a_j0)
    {
        d0_ = a_d0;
        s0_ = a_s0;
        j0_ = a_j0;
    }

    void SplineC3xS3
                ::setEndConds(double a_d3, double a_s3)
    {
        d3_ = a_d3;
        s3_ = a_s3;
    }

    void SplineC3xS3
                ::computeAndSaveTo(std::vector<Degree6Polynomial>& results)
    {
        s3e_.clear();
        s3e_.addEquation1(s3p0_.valueAt(0.), y0_);
        s3e_.addEquation1(s3p0_.valueAt(h0_), y1_);
        s3e_.addEquation1(s3p0_.derivativeAt(0.), d0_);
        s3e_.addEquation2(s3p0_.derivativeAt(h0_), s3p1_.derivativeAt(0.));
        s3e_.addEquation1(s3p0_.secondDerivativeAt(0.), s0_);
        s3e_.addEquation1(s3p0_.thirdDerivativeAt(0.), j0_);
        s3e_.addEquation2(s3p0_.secondDerivativeAt(h0_), s3p1_.secondDerivativeAt(0.));
        s3e_.addEquation2(s3p0_.thirdDerivativeAt(h0_), s3p1_.thirdDerivativeAt(0.));
        s3e_.addEquation1(s3p1_.valueAt(0.), y1_);
        s3e_.addEquation1(s3p1_.valueAt(h1_), y2_);
        s3e_.addEquation2(s3p1_.derivativeAt(h1_), s3p2_.derivativeAt(0.));
        s3e_.addEquation2(s3p1_.secondDerivativeAt(h1_), s3p2_.secondDerivativeAt(0.));
        s3e_.addEquation2(s3p1_.thirdDerivativeAt(h1_), s3p2_.thirdDerivativeAt(0.));
        s3e_.addEquation1(s3p2_.valueAt(0.), y2_);
        s3e_.addEquation1(s3p2_.valueAt(h2_), y3_);
        s3e_.addEquation1(s3p2_.derivativeAt(h2_), d3_);
        s3e_.addEquation1(s3p2_.secondDerivativeAt(h2_), s3_);
        s3e_.solve();
        results.push_back(s3e_.buildDegree6Polynomial(s3p0_));
        results.push_back(s3e_.buildDegree6Polynomial(s3p1_));
        results.push_back(s3e_.buildDegree6Polynomial(s3p2_));
    }

///////////////////////////////////////////////////////////////////////////////
// class SplineC3xS2                                                         //
///////////////////////////////////////////////////////////////////////////////

    SplineC3xS2
                ::SplineC3xS2()
    : s2c_()
    , s2p0_(s2c_.addPolynomial(fnGetEnvIntWithDefault("ST_C3xS2_D0", 4)))
    , s2p1_(s2c_.addPolynomial(fnGetEnvIntWithDefault("ST_C3xS2_D1", 4)))
    , s2e_(s2c_)
    , y0_(NAN), y1_(NAN), y2_(NAN), h0_(NAN), h1_(NAN), d0_(NAN), d2_(NAN), s0_(NAN), s2_(NAN), j0_(NAN)
    {}

    void SplineC3xS2
                ::setXs(double a_x0, double a_x1, double a_x2)
    {
        h0_ = a_x1 - a_x0;
        h1_ = a_x2 - a_x1;
    }

    void SplineC3xS2
                ::setYs(double a_y0, double a_y1, double a_y2)
    {
        y0_ = a_y0;
        y1_ = a_y1;
        y2_ = a_y2;
#ifdef DEBUG_PRINT_SPLINE_COMPUTATION
        std::cout << "Spline computation: " << y0_ << " ~ " << y1_ << " ~ " << y2_ << std::endl;
#endif
    }

    void SplineC3xS2
                ::setStartConds(double a_d0, double a_s0, double a_j0)
    {
        d0_ = a_d0;
        s0_ = a_s0;
        j0_ = a_j0;
    }

    void SplineC3xS2
                ::setEndConds(double a_d2, double a_s2)
    {
        d2_ = a_d2;
        s2_ = a_s2;
    }

    void SplineC3xS2
                ::computeAndSaveTo(std::vector<Degree6Polynomial>& results)
    {
        s2e_.clear();
        s2e_.addEquation1(s2p0_.valueAt(0.), y0_);
        s2e_.addEquation1(s2p0_.valueAt(h0_), y1_);
        s2e_.addEquation1(s2p0_.derivativeAt(0.), d0_);
        s2e_.addEquation2(s2p0_.derivativeAt(h0_), s2p1_.derivativeAt(0.));
        s2e_.addEquation1(s2p0_.secondDerivativeAt(0.), s0_);
        s2e_.addEquation2(s2p0_.secondDerivativeAt(h0_), s2p1_.secondDerivativeAt(0.));
        s2e_.addEquation1(s2p0_.thirdDerivativeAt(0.), j0_);
        s2e_.addEquation2(s2p0_.thirdDerivativeAt(h0_), s2p1_.thirdDerivativeAt(0.));
        s2e_.addEquation1(s2p1_.valueAt(0.), y1_);
        s2e_.addEquation1(s2p1_.valueAt(h1_), y2_);
        s2e_.addEquation1(s2p1_.derivativeAt(h1_), d2_);
        s2e_.addEquation1(s2p1_.secondDerivativeAt(h1_), s2_);
        s2e_.solve();
        results.push_back(s2e_.buildDegree6Polynomial(s2p0_));
        results.push_back(s2e_.buildDegree6Polynomial(s2p1_));
    }

///////////////////////////////////////////////////////////////////////////////
// class SplineC3xS1                                                         //
///////////////////////////////////////////////////////////////////////////////

    SplineC3xS1
                ::SplineC3xS1()
    : s1c_()
    , s1p0_(s1c_.addPolynomial(fnGetEnvIntWithDefault("ST_C3xS1_D0", 6)))
    , s1e_(s1c_)
    , y0_(NAN), y1_(NAN), h0_(NAN), d0_(NAN), d1_(NAN), s0_(NAN), s1_(NAN), j0_(NAN)
    {}

    void SplineC3xS1
                ::setXs(double a_x0, double a_x1)
    {
        h0_ = a_x1 - a_x0;
    }

    void SplineC3xS1
                ::setYs(double a_y0, double a_y1)
    {
        y0_ = a_y0;
        y1_ = a_y1;
#ifdef DEBUG_PRINT_SPLINE_COMPUTATION
        std::cout << "Spline computation: " << y0_ << " ~ " << y1_ << std::endl;
#endif
    }

    void SplineC3xS1
                ::setStartConds(double a_d0, double a_s0, double a_j0)
    {
        d0_ = a_d0;
        s0_ = a_s0;
        j0_ = a_j0;
    }

    void SplineC3xS1
                ::setEndConds(double a_d1, double a_s1)
    {
        d1_ = a_d1;
        s1_ = a_s1;
    }

    void SplineC3xS1
                ::computeAndSaveTo(std::vector<Degree6Polynomial>& results)
    {
        s1e_.clear();
        s1e_.addEquation1(s1p0_.valueAt(0.), y0_);
        s1e_.addEquation1(s1p0_.valueAt(h0_), y1_);
        s1e_.addEquation1(s1p0_.derivativeAt(0.), d0_);
        s1e_.addEquation1(s1p0_.derivativeAt(h0_), d1_);
        s1e_.addEquation1(s1p0_.secondDerivativeAt(0.), s0_);
        s1e_.addEquation1(s1p0_.secondDerivativeAt(h0_), s1_);
        s1e_.addEquation1(s1p0_.thirdDerivativeAt(0.), j0_);
        s1e_.solve();
        results.push_back(s1e_.buildDegree6Polynomial(s1p0_));
    }

///////////////////////////////////////////////////////////////////////////////
// class SplineC1S2I                                                         //
///////////////////////////////////////////////////////////////////////////////

    SplineC1S2I
                ::SplineC1S2I()
    : c_()
    , p0_(c_.addPolynomial(3))
    , p1_(c_.addPolynomial(3))
    , e_(c_)
    , y0_(NAN), y1_(NAN), y2_(NAN), h0_(NAN), h1_(NAN), d0_(NAN), d1_(NAN), d2_(NAN)
    {}

    void SplineC1S2I
                ::setXs(double a_x0, double a_x1, double a_x2)
    {
        h0_ = a_x1 - a_x0;
        h1_ = a_x2 - a_x1;
    }

    void SplineC1S2I
                ::setYs(double a_y0, double a_y1, double a_y2)
    {
        y0_ = a_y0;
        y1_ = a_y1;
        y2_ = a_y2;
#ifdef DEBUG_PRINT_SPLINE_COMPUTATION
        std::cout << "Spline computation: " << y0_ << " ~ " << y1_ << " ~ " << y2_ << std::endl;
#endif
    }

    void SplineC1S2I
                ::setDerivatives(double a_d0, double a_d1, double a_d2)
    {
        d0_ = a_d0;
        d1_ = a_d1;
        d2_ = a_d2;
    }

    void SplineC1S2I
                ::computeAndSaveTo(Degree3Polynomial& result0, Degree3Polynomial& result1)
    {
        e_.clear();
        e_.addEquation1(p0_.valueAt(0.), y0_);
        e_.addEquation1(p0_.valueAt(h0_), y1_);
        e_.addEquation1(p0_.derivativeAt(0.), d0_);
        e_.addEquation1(p0_.derivativeAt(h0_), d1_);
        e_.addEquation1(p1_.valueAt(0.), y1_);
        e_.addEquation1(p1_.valueAt(h1_), y2_);
        e_.addEquation1(p1_.derivativeAt(0.), d1_);
        e_.addEquation1(p1_.derivativeAt(h1_), d2_);
        e_.solve();
        result0 = e_.buildDegree3Polynomial(p0_);
        result1 = e_.buildDegree3Polynomial(p1_);
    }
        
}
