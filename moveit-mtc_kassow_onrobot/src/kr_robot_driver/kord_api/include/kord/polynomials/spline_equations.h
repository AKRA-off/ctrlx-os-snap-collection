/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2024, KR2013ApS
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

#ifndef WEAVING_GENERATOR_SPLINE_EQUATIONS_H
#define WEAVING_GENERATOR_SPLINE_EQUATIONS_H

#include <kord/polynomials/polynomials.h>

#include <eigen3/Eigen/Dense>

namespace kswx_weaving_generator {

class CoefficientSet;

/// \brief generates matrix coefficients for condition on value of polynomial
class PolynomialValueAt {
    friend class Polynomial;
    size_t variables_begin_;
    size_t variables_end_;
    double argument_;
    PolynomialValueAt(size_t a_variables_begin, size_t a_variables_end, double a_argument);

public:
    size_t variablesBegin() const;
    size_t variablesEnd() const;

    double getCoefficient(size_t a_variable) const;
    double getConstant() const;
};

/// \brief generates matrix coefficients for condition on derivative of polynomial
class PolynomialDerivativeAt {
    friend class Polynomial;
    size_t variables_begin_;
    size_t variables_end_;
    double argument_;
    PolynomialDerivativeAt(size_t a_variables_begin, size_t a_variables_end, double a_argument);

public:
    size_t variablesBegin() const;
    size_t variablesEnd() const;

    double getCoefficient(size_t a_variable) const;
    double getConstant() const;
};

/// \brief generates matrix coefficients for condition on second derivative of polynomial
class PolynomialSecondDerivativeAt {
    friend class Polynomial;
    size_t variables_begin_;
    size_t variables_end_;
    double argument_;
    PolynomialSecondDerivativeAt(size_t a_variables_begin, size_t a_variables_end, double a_argument);

public:
    size_t variablesBegin() const;
    size_t variablesEnd() const;

    double getCoefficient(size_t a_variable) const;
    double getConstant() const;
};

/// \brief generates matrix coefficients for condition on third derivative of polynomial
class PolynomialThirdDerivativeAt {
    friend class Polynomial;
    size_t variables_begin_;
    size_t variables_end_;
    double argument_;
    PolynomialThirdDerivativeAt(size_t a_variables_begin, size_t a_variables_end, double a_argument);

public:
    size_t variablesBegin() const;
    size_t variablesEnd() const;

    double getCoefficient(size_t a_variable) const;
    double getConstant() const;
};

/// \brief represent a polynomial in a system of linear equations
class Polynomial {
    friend class CoefficientSet;

    const CoefficientSet *coefficient_set_;
    size_t variables_begin_;
    size_t variables_end_;

    Polynomial(CoefficientSet *a_coefficient_set, size_t a_variables_begin, size_t a_variables_end);

public:
    // this constructor gives an invalid state and should be used only when
    // initialization is needed and overridden by
    // CoefficientSet::addPolynomial before any usage
    Polynomial();

    size_t variablesBegin() const;
    size_t variablesEnd() const;
    size_t degree() const;

    PolynomialValueAt valueAt(double a_argument) const;
    PolynomialDerivativeAt derivativeAt(double a_argument) const;
    PolynomialSecondDerivativeAt secondDerivativeAt(double a_argument) const;
    PolynomialThirdDerivativeAt thirdDerivativeAt(double a_argument) const;
};

/// \brief represents unknowns in a system of linear equations
class CoefficientSet {
    size_t variables_count_ = 0;

public:
    size_t variablesCount() const;
    Polynomial addPolynomial(int a_degree);
};

/// \brief represent a system of linear equations
class EquationSet {
    size_t dimension_;
    size_t equations_;
    Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> linear_; // linear coefficients on the left side of equations
    Eigen::Matrix<double, Eigen::Dynamic, 1> constant_;            // on the right side of equations
    Eigen::Matrix<double, 1, Eigen::Dynamic> results_;             // vector for storing results
public:
    EquationSet(const CoefficientSet &coefficient_set);

    void init(const CoefficientSet &coefficient_set);
    void clear();

    template <typename G1> void addEquation1(const G1 &left_side_generator, double right_side);
    template <typename G1, typename G2> void addEquation2(const G1 &left_side_generator, const G2 &right_side_generator);

    void solve();
    double getCoefficient(const Polynomial &a_polynomial, size_t a_power) const;
    Degree3Polynomial buildDegree3Polynomial(const Polynomial &a_polynomial) const;
    Degree5Polynomial buildDegree5Polynomial(const Polynomial &a_polynomial) const;
    Degree6Polynomial buildDegree6Polynomial(const Polynomial &a_polynomial) const;
    Degree7Polynomial buildDegree7Polynomial(const Polynomial &a_polynomial) const;
};

template <typename G1> void EquationSet ::addEquation1(const G1 &left_side_generator, double right_side)
{
    if (equations_ >= dimension_) {
        return; /* assert(false); */
    }
    for (size_t var = 0; var < dimension_; var++) {
        linear_(equations_, var) = 0.;
    }
    constant_(equations_) = 0.;

    for (size_t var = left_side_generator.variablesBegin(); var < left_side_generator.variablesEnd(); var++) {
        linear_(equations_, var) += left_side_generator.getCoefficient(var);
    }
    constant_(equations_) -= left_side_generator.getConstant();
    constant_(equations_) += right_side;
    equations_++;
}

template <typename G1, typename G2>
void EquationSet ::addEquation2(const G1 &left_side_generator, const G2 &right_side_generator)
{
    if (equations_ >= dimension_) {
        return; /* assert(false); */
    }
    for (size_t var = 0; var < dimension_; var++) {
        linear_(equations_, var) = 0.;
    }
    constant_(equations_) = 0.;

    for (size_t var = left_side_generator.variablesBegin(); var < left_side_generator.variablesEnd(); var++) {
        linear_(equations_, var) += left_side_generator.getCoefficient(var);
    }
    constant_(equations_) -= left_side_generator.getConstant();
    for (size_t var = right_side_generator.variablesBegin(); var < right_side_generator.variablesEnd(); var++) {
        linear_(equations_, var) -= right_side_generator.getCoefficient(var);
    }
    constant_(equations_) += right_side_generator.getConstant();
    equations_++;
}

} // namespace kswx_weaving_generator

#endif // WEAVING_GENERATOR_SPLINE_EQUATIONS_H
