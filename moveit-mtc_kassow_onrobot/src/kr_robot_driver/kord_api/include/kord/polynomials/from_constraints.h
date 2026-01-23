/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2024, KR Soft s.r.o.
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
 *   * Neither the name of the Kassow Robots nor the names of its
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

#ifndef WEAVING_GENERATOR_FROM_CONSTRAINTS_H
#define WEAVING_GENERATOR_FROM_CONSTRAINTS_H

#include <kord/polynomials/polynomials.h>

#include <cmath>

namespace kswx_weaving_generator
{
    /**
     * \brief computes cubic polynomial having specific value and derivative in points 0. and 1.
     * 
     * use PolynomialFromConstraints_C1At2Points_0A for point 0. and arbitrary point
     * use PolynomialFromConstraints_C1At2Points_AA for arbitrary points
     */
    class PolynomialFromConstraints_C1At2Points_01
    {
        double v0_ = NAN;
        double v1_ = NAN;
        double d0_ = NAN;
        double d1_ = NAN;
    public:
        PolynomialFromConstraints_C1At2Points_01();

        PolynomialFromConstraints_C1At2Points_01 &setValues(double a_v0, double a_v1);

        PolynomialFromConstraints_C1At2Points_01 &setDerivatives(double a_d0, double a_d1);

        [[nodiscard]] Degree3Polynomial compute() const;
    };

    /**
     * \brief computes cubic polynomial having specific value and derivative in point 0. and arbitrary point
     * 
     * use PolynomialFromConstraints_C1At2Points_01 for points 0. and 1.
     * use PolynomialFromConstraints_C1At2Points_AA for arbitrary points
     */
    class PolynomialFromConstraints_C1At2Points_0A
    {
        PolynomialFromConstraints_C1At2Points_01 parent_;
        double p1_ = NAN;
    public:
        PolynomialFromConstraints_C1At2Points_0A();

        PolynomialFromConstraints_C1At2Points_0A &setPoint(double a_p1);

        PolynomialFromConstraints_C1At2Points_0A &setValues(double a_v0, double a_v1);

        PolynomialFromConstraints_C1At2Points_0A &setDerivatives(double a_d0, double a_d1);

        [[nodiscard]] Degree3Polynomial compute() const;
    };

    /**
     * \brief computes cubic polynomial having specific value and derivative two arbitrary point
     * 
     * use PolynomialFromConstraints_C1At2Points_01 for points 0. and 1.
     * use PolynomialFromConstraints_C1At2Points_0A for point 0. and arbitrary point
     */
    class PolynomialFromConstraints_C1At2Points_AA
    {
        PolynomialFromConstraints_C1At2Points_01 parent_;
        double p0_ = NAN;
        double p1_ = NAN;
    public:
        PolynomialFromConstraints_C1At2Points_AA();

        PolynomialFromConstraints_C1At2Points_AA &setPoints(double a_p0, double a_p1);

        PolynomialFromConstraints_C1At2Points_AA &setValues(double a_v0, double a_v1);

        PolynomialFromConstraints_C1At2Points_AA &setDerivatives(double a_d0, double a_d1);

        [[nodiscard]] Degree3Polynomial compute() const;
    };

    /**
     * \brief computes cubic polynomial having specific value and derivative in points 0. and 1.
     * 
     * use PolynomialFromConstraints_C2At2Points_0A for point 0. and arbitrary point
     * use PolynomialFromConstraints_C2At2Points_AA for arbitrary points
     */
    class PolynomialFromConstraints_C2At2Points_01
    {
        double v0_ = NAN;
        double v1_ = NAN;
        double d0_ = NAN;
        double d1_ = NAN;
        double s0_ = NAN;
        double s1_ = NAN;
    public:
        PolynomialFromConstraints_C2At2Points_01();

        PolynomialFromConstraints_C2At2Points_01 &setValues(double a_v0, double a_v1);

        PolynomialFromConstraints_C2At2Points_01 &setDerivatives(double a_d0, double a_d1);

        PolynomialFromConstraints_C2At2Points_01 &setSecondDerivatives(double a_s0, double a_s1);

        [[nodiscard]] Degree5Polynomial compute() const;
    };

    /**
     * \brief computes cubic polynomial having specific value and derivative in point 0. and arbitrary point
     * 
     * use PolynomialFromConstraints_C2At2Points_01 for points 0. and 1.
     * use PolynomialFromConstraints_C2At2Points_AA for arbitrary points
     */
    class PolynomialFromConstraints_C2At2Points_0A
    {
        PolynomialFromConstraints_C2At2Points_01 parent_;
        double p1_ = NAN;
    public:
        PolynomialFromConstraints_C2At2Points_0A();

        PolynomialFromConstraints_C2At2Points_0A &setPoint(double a_p1);

        PolynomialFromConstraints_C2At2Points_0A &setValues(double a_v0, double a_v1);

        PolynomialFromConstraints_C2At2Points_0A &setDerivatives(double a_d0, double a_d1);

        PolynomialFromConstraints_C2At2Points_0A &setSecondDerivatives(double a_s0, double a_s1);

        [[nodiscard]] Degree5Polynomial compute() const;
    };

    /**
     * \brief computes cubic polynomial having specific value and derivative two arbitrary point
     * 
     * use PolynomialFromConstraints_C2At2Points_01 for points 0. and 1.
     * use PolynomialFromConstraints_C2At2Points_0A for point 0. and arbitrary point
     */
    class PolynomialFromConstraints_C2At2Points_AA
    {
        PolynomialFromConstraints_C2At2Points_01 parent_;
        double p0_ = NAN;
        double p1_ = NAN;
    public:
        PolynomialFromConstraints_C2At2Points_AA();

        PolynomialFromConstraints_C2At2Points_AA &setPoints(double a_p0, double a_p1);

        PolynomialFromConstraints_C2At2Points_AA &setValues(double a_v0, double a_v1);

        PolynomialFromConstraints_C2At2Points_AA &setDerivatives(double a_d0, double a_d1);

        PolynomialFromConstraints_C2At2Points_AA &setSecondDerivatives(double a_s0, double a_s1);

        [[nodiscard]] Degree5Polynomial compute() const;
    };

    /**
     * \brief computes cubic polynomial having specific value and derivative in points 0. and 1.
     * 
     * use PolynomialFromConstraints_C3At2Points_0A for point 0. and arbitrary point
     * use PolynomialFromConstraints_C3At2Points_AA for arbitrary points
     */
    class PolynomialFromConstraints_C3At2Points_01
    {
        double v0_ = NAN;
        double v1_ = NAN;
        double d0_ = NAN;
        double d1_ = NAN;
        double s0_ = NAN;
        double s1_ = NAN;
        double t0_ = NAN;
        double t1_ = NAN;
    public:
        PolynomialFromConstraints_C3At2Points_01();

        PolynomialFromConstraints_C3At2Points_01 &setValues(double a_v0, double a_v1);

        PolynomialFromConstraints_C3At2Points_01 &setDerivatives(double a_d0, double a_d1);

        PolynomialFromConstraints_C3At2Points_01 &setSecondDerivatives(double a_s0, double a_s1);

        PolynomialFromConstraints_C3At2Points_01 &setThirdDerivatives(double a_t0, double a_t1);

        [[nodiscard]] Degree7Polynomial compute() const;

        [[nodiscard]] double v0() const { return v0_; }

        [[nodiscard]] double v1() const { return v1_; }

        [[nodiscard]] double d0() const { return d0_; }

        [[nodiscard]] double d1() const { return d1_; }

        [[nodiscard]] double s0() const { return s0_; }

        [[nodiscard]] double s1() const { return s1_; }

        [[nodiscard]] double t0() const { return t0_; }

        [[nodiscard]] double t1() const { return t1_; }
    };

    /**
     * \brief computes cubic polynomial having specific value and derivative in point 0. and arbitrary point
     * 
     * use PolynomialFromConstraints_C3At2Points_01 for points 0. and 1.
     * use PolynomialFromConstraints_C3At2Points_AA for arbitrary points
     */
    class PolynomialFromConstraints_C3At2Points_0A
    {
        PolynomialFromConstraints_C3At2Points_01 parent_;
        double p1_ = NAN;
    public:
        PolynomialFromConstraints_C3At2Points_0A();

        PolynomialFromConstraints_C3At2Points_0A &setPoint(double a_p1);

        PolynomialFromConstraints_C3At2Points_0A &setValues(double a_v0, double a_v1);

        PolynomialFromConstraints_C3At2Points_0A &setDerivatives(double a_d0, double a_d1);

        PolynomialFromConstraints_C3At2Points_0A &setSecondDerivatives(double a_s0, double a_s1);

        PolynomialFromConstraints_C3At2Points_0A &setThirdDerivatives(double a_t0, double a_t1);

        [[nodiscard]] Degree7Polynomial compute() const;
    };

    /**
     * \brief computes cubic polynomial having specific value and derivative two arbitrary point
     * 
     * use PolynomialFromConstraints_C3At2Points_01 for points 0. and 1.
     * use PolynomialFromConstraints_C3At2Points_0A for point 0. and arbitrary point
     */
    class PolynomialFromConstraints_C3At2Points_AA
    {
        PolynomialFromConstraints_C3At2Points_01 parent_;
        double p0_ = NAN;
        double p1_ = NAN;
    public:
        PolynomialFromConstraints_C3At2Points_AA();

        PolynomialFromConstraints_C3At2Points_AA &setPoints(double a_p0, double a_p1);

        PolynomialFromConstraints_C3At2Points_AA &setValues(double a_v0, double a_v1);

        PolynomialFromConstraints_C3At2Points_AA &setDerivatives(double a_d0, double a_d1);

        PolynomialFromConstraints_C3At2Points_AA &setSecondDerivatives(double a_s0, double a_s1);

        PolynomialFromConstraints_C3At2Points_AA &setThirdDerivatives(double a_t0, double a_t1);

        [[nodiscard]] Degree7Polynomial compute() const;
    };
}; // namespace

#endif // WEAVING_GENERATOR_FROM_CONSTRAINTS_H
