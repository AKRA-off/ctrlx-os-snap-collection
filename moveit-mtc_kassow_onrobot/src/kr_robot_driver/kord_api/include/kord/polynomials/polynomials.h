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

#ifndef WEAVING_GENERATOR_POLYNOMIALS_H
#define WEAVING_GENERATOR_POLYNOMIALS_H

#include <string>
#include <utility>

namespace kswx_weaving_generator
{

    /// 0 == k0 + k1*x
    double fnGeneralLinearEquation(double k0, double k1, double min, double max);

    /// 0 == k0 + k1*x + 1*x*x
    double fnNormalizedQuadraticEquation(double k0, double k1, double min, double max);

    /// 0 == k0 + k1*x + k2*x*x
    double fnGeneralQuadraticEquation(double k0, double k1, double k2, double min, double max);

    /// 0 == k0 + 0*k1 + k2*x*x + 0*x*x*x + 1*x*x*x*x
    double fnNormalizedBiquadraticEquation(double k0, double k2, double min, double max);

    /// 0 == k0 + k1*x + 0*x*x + 1*x*x*x
    double fnDepressedCubicEquation(double k0, double k1, double min, double max);

    /// 0 == k0 + k1*x + k2*x*x + 1*x*x*x
    double fnNormalizedCubicEquation(double k0, double k1, double k2, double min, double max);

    /// 0 == k0 + k1*x + k2*x*x + k3*x*x*x
    double fnGeneralCubicEquation(double k0, double k1, double k2, double k3, double min, double max);

    /// 0 == k0 + k1*x + k2*x*x + 0*x*x*x + 1*x*x*x*x
    double fnDepressedQuarticEquation(double k0, double k1, double k2, double min, double max);

    /// 0 == k0 + k1*x + k2*x*x + k3*x*x*x + 1*x*x*x*x
    double fnNormalizedQuarticEquation(double k0, double k1, double k2, double k3, double min, double max);

    /// 0 == k0 + k1*x + k2*x*x + k3*x*x*x + k4*x*x*x*x
    double fnGeneralQuarticEquation(double k0, double k1, double k2, double k3, double k4,
                                    double min, double max);

    double fnGeneralQuarticEquationNotNan(double k0, double k1, double k2, double k3, double k4,
                                          double min, double max,
                                          bool a_log_if_nan);

    class Degree3Polynomial;
    class Degree4Polynomial;
    class Degree5Polynomial;
    class Degree6Polynomial;

    /**
    * \brief implements a degree 1 polynomial
    * 
    * implements function x -> a + b*x
    */
    class Degree1Polynomial
    {
    public:
        Degree1Polynomial();
        Degree1Polynomial(double a_a, double a_b);
        
        [[nodiscard]] double root() const;

        [[maybe_unused]] [[nodiscard]] std::string toString() const;
    private:
        double a_;
        double b_;
    };

    /**
    * \brief implements a degree 2 polynomial
    * 
    * implements function x -> a + b*x + c*x*x
    */
    class Degree2Polynomial
    {
    public:
        Degree2Polynomial();
        Degree2Polynomial(double a_a, double a_b, double a_c);
        
        [[nodiscard]] double valueAt(double x) const;
        [[nodiscard]] double derivativeAt(double x) const;
        [[nodiscard]] double secondDerivativeAt(double x) const;
        [[nodiscard]] double thirdDerivativeAt(double x) const;
        [[nodiscard]] double fourthDerivativeAt(double x) const;

        [[nodiscard]] Degree1Polynomial derivative() const;
        [[nodiscard]] Degree3Polynomial integrate() const;

        /**
         * \brief shifts the polynomial up so that new(x) = old(x) + a_shift
         */
        void yShift(double a_shift);
        /**
         * \brief stretches the polynomial so that new(x) = old(x) * a_stretch
         */
        void yStretch(double a_stretch);
        /**
         * \brief stretches the polynomial so that new(x) = old(x / a_stretch)
         */
        void xStretch(double a_stretch);
        /**
         * \brief shifts the polynomial left so that new(x) = old(x + a_shift)
         */
        void xShift(double a_shift);

        [[nodiscard]] double equalsConstantAt(double y, double x_min, double x_max) const;
        [[nodiscard]] double maximumOnInterval(double a_xmin, double a_xmax) const;
        [[nodiscard]] double minimumOnInterval(double a_xmin, double a_xmax) const;
        
        [[nodiscard]] std::pair<double, double> roots() const;
        
        [[nodiscard]] std::string toString() const;
    private:
        double a_;
        double b_;
        double c_;
    };

    /**
    * \brief implements a degree 3 polynomial
    * 
    * implements function x -> a + b*x + c*x*x + d*x*x*x
    */
    class Degree3Polynomial
    {
    public:
        Degree3Polynomial();
        Degree3Polynomial(double a_a, double a_b, double a_c, double a_d);
        
        [[nodiscard]] double valueAt(double x) const;
        [[nodiscard]] double derivativeAt(double x) const;
        [[nodiscard]] double secondDerivativeAt(double x) const;
        [[nodiscard]] double thirdDerivativeAt( double x) const;
        [[nodiscard]] double fourthDerivativeAt( double x) const;
        
        [[nodiscard]] Degree2Polynomial derivative() const;
        [[nodiscard]] Degree4Polynomial integrate() const;

        [[nodiscard]] double maximumOnInterval(double a_xmin, double a_xmax) const;
        [[nodiscard]] double minimumOnInterval(double a_xmin, double a_xmax) const;
        /**
         * \brief shifts the polynomial up so that new(x) = old(x) + a_shift
         */
        void yShift(double a_shift);
        /**
         * \brief stretches the polynomial so that new(x) = old(x) * a_stretch
         */
        void yStretch(double a_stretch);
        /**
         * \brief stretches the polynomial so that new(x) = old(x / a_stretch)
         */
        void xStretch(double a_stretch);
        /**
         * \brief shifts the polynomial left so that new(x) = old(x + a_shift)
         */
        void xShift(double a_shift);
        [[nodiscard]] double equalsConstantAt(double y, double x_min, double x_max) const;
        [[nodiscard]] std::string toString() const;
    private:
        double a_;
        double b_;
        double c_;
        double d_;
    };

    class Degree4Polynomial
    {
    public:
        Degree4Polynomial();
        Degree4Polynomial(double a_a, double a_b, double a_c, double a_d, double a_e);
        [[nodiscard]] double valueAt(double x) const;
        [[nodiscard]] double derivativeAt(double x) const;
        [[nodiscard]] double secondDerivativeAt(double x) const;
        [[nodiscard]] double thirdDerivativeAt(double x) const;
        [[nodiscard]] double fourthDerivativeAt(double x) const;

        [[nodiscard]] Degree3Polynomial derivative() const;
        [[nodiscard]] Degree5Polynomial extendDegree() const;

        [[nodiscard]] double maximumOnInterval(double a_xmin, double a_xmax) const;
        [[nodiscard]] double minimumOnInterval(double a_xmin, double a_xmax) const;

        /**
         * \brief shifts the polynomial left so that new(x) = old(x + a_shift)
         */
        void xShift(double a_shift);
        /**
         * \brief shifts the polynomial up so that new(x) = old(x) + a_shift
         */
        void yShift(double a_shift);
        /**
         * \brief stretches the polynomial so that new(x) = old(x / a_stretch)
         */
        void xStretch(double a_stretch);
        /**
         * \brief stretches the polynomial so that new(x) = old(x) * a_stretch
         */
        void yStretch(double a_stretch);

        [[nodiscard]] double equalsConstantAt(double y, double x_min, double x_max, bool a_log_if_nan) const;
        
        [[nodiscard]] std::string toString() const;
        
        [[nodiscard]] double coef_0() const { return a_; }
        [[nodiscard]] double coef_1() const { return b_; }
        [[nodiscard]] double coef_2() const { return c_; }
        [[nodiscard]] double coef_3() const { return d_; }
        [[nodiscard]] double coef_4() const { return e_; }
    private:
        double a_;
        double b_;
        double c_;
        double d_;
        double e_;
    };

    class Degree5Polynomial
    {
    public:
        Degree5Polynomial();
        Degree5Polynomial(double a_a, double a_b, double a_c, double a_d, double a_e, double a_f);

        [[nodiscard]] double valueAt(double x) const;
        [[nodiscard]] double derivativeAt(double x) const;
        [[nodiscard]] double secondDerivativeAt(double x) const;
        [[nodiscard]] double thirdDerivativeAt(double x) const;
        [[nodiscard]] double fourthDerivativeAt(double x) const;

        [[nodiscard]] Degree4Polynomial derivative() const;
        [[nodiscard]] Degree6Polynomial integrate(double x) const;

        /**
         * \brief shifts the polynomial left so that new(x) = old(x + a_shift)
         */
        void xShift(double a_shift);
        /**
         * \brief shifts the polynomial up so that new(x) = old(x) + a_shift
         */
        void yShift(double a_shift);
        /**
         * \brief stretches the polynomial so that new(x) = old(x / a_stretch)
         */
        void xStretch(double a_stretch);
        /**
         * \brief stretches the polynomial so that new(x) = old(x) * a_stretch
         */
        void yStretch(double a_stretch);
        [[nodiscard]] std::string toString() const;
    private:
        double a_;
        double b_;
        double c_;
        double d_;
        double e_;
        double f_;
    };

    class Degree6Polynomial
    {
    public:
        Degree6Polynomial();
        Degree6Polynomial(double a_a, double a_b, double a_c, double a_d, double a_e, double a_f, double a_g);
        [[nodiscard]] double valueAt(double x) const;
        [[nodiscard]] double derivativeAt(double x) const;
        [[nodiscard]] double secondDerivativeAt(double x) const;
        [[nodiscard]] double thirdDerivativeAt(double x) const;
        [[nodiscard]] double fourthDerivativeAt(double x) const;
        [[nodiscard]] std::string toString() const;
    private:
        double a_;
        double b_;
        double c_;
        double d_;
        double e_;
        double f_;
        double g_;
    };

    class Degree7Polynomial
    {
    public:
        Degree7Polynomial();
        Degree7Polynomial(double a_a, double a_b, double a_c, double a_d, double a_e, double a_f, double a_g, double a_h);
        [[nodiscard]] double valueAt(double x) const;
        [[nodiscard]] double derivativeAt(double x) const;
        [[nodiscard]] double secondDerivativeAt(double x) const;
        [[nodiscard]] double thirdDerivativeAt(double x) const;
        [[nodiscard]] double fourthDerivativeAt(double x) const;

        /**
         * \brief shifts the polynomial left so that new(x) = old(x + a_shift)
         */
        void xShift(double a_shift);
        /**
         * \brief shifts the polynomial up so that new(x) = old(x) + a_shift
         */
        void yShift(double a_shift);
        /**
         * \brief stretches the polynomial so that new(x) = old(x / a_stretch)
         */
        void xStretch(double a_stretch);
        /**
         * \brief stretches the polynomial so that new(x) = old(x) * a_stretch
         */
        void yStretch(double a_stretch);

        [[nodiscard]] std::string toString() const;
    private:
        double a_;
        double b_;
        double c_;
        double d_;
        double e_;
        double f_;
        double g_;
        double h_;
    };

}; // namespace

#endif // WEAVING_GENERATOR_POLYNOMIALS_H
