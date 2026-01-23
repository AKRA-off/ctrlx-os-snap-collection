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

#include <kord/polynomials/spline_equations.h>

#include <cassert>

namespace kswx_weaving_generator {

///////////////////////////////////////////////////////////////////////////////
// class PolynomialValueAt                                                   //
///////////////////////////////////////////////////////////////////////////////

PolynomialValueAt ::PolynomialValueAt(size_t a_variables_begin, size_t a_variables_end, double a_argument)
    : variables_begin_(a_variables_begin), variables_end_(a_variables_end), argument_(a_argument)
{
}

size_t PolynomialValueAt ::variablesBegin() const { return variables_begin_; }

size_t PolynomialValueAt ::variablesEnd() const { return variables_end_; }

double PolynomialValueAt ::getCoefficient(size_t a_variable) const
{
    size_t degree = a_variable - variables_begin_;
    double result = 1.;
    for (size_t i = 0; i < degree; i++) {
        result *= argument_;
    }
    return result;
}

double PolynomialValueAt ::getConstant() const { return 0.; }

///////////////////////////////////////////////////////////////////////////////
// class PolynomialDerivativeAt                                              //
///////////////////////////////////////////////////////////////////////////////

PolynomialDerivativeAt ::PolynomialDerivativeAt(size_t a_variables_begin, size_t a_variables_end, double a_argument)
    : variables_begin_(a_variables_begin), variables_end_(a_variables_end), argument_(a_argument)
{
}

size_t PolynomialDerivativeAt ::variablesBegin() const { return variables_begin_; }

size_t PolynomialDerivativeAt ::variablesEnd() const { return variables_end_; }

double PolynomialDerivativeAt ::getCoefficient(size_t a_variable) const
{
    size_t degree = a_variable - variables_begin_;
    double result = 1.;
    for (size_t i = 1; i < degree; i++) {
        result *= argument_;
    }
    result *= (double)degree;
    return result;
}

double PolynomialDerivativeAt ::getConstant() const { return 0.; }

///////////////////////////////////////////////////////////////////////////////
// class PolynomialSecondDerivativeAt                                        //
///////////////////////////////////////////////////////////////////////////////

PolynomialSecondDerivativeAt ::PolynomialSecondDerivativeAt(size_t a_variables_begin,
                                                            size_t a_variables_end,
                                                            double a_argument)
    : variables_begin_(a_variables_begin), variables_end_(a_variables_end), argument_(a_argument)
{
}

size_t PolynomialSecondDerivativeAt ::variablesBegin() const { return variables_begin_; }

size_t PolynomialSecondDerivativeAt ::variablesEnd() const { return variables_end_; }

double PolynomialSecondDerivativeAt ::getCoefficient(size_t a_variable) const
{
    size_t degree = a_variable - variables_begin_;
    double result = 1.;
    for (size_t i = 2; i < degree; i++) {
        result *= argument_;
    }
    result *= (double)degree * (double)(degree - 1);
    return result;
}

double PolynomialSecondDerivativeAt ::getConstant() const { return 0.; }

///////////////////////////////////////////////////////////////////////////////
// class PolynomialThirdDerivativeAt                                         //
///////////////////////////////////////////////////////////////////////////////

PolynomialThirdDerivativeAt ::PolynomialThirdDerivativeAt(size_t a_variables_begin,
                                                          size_t a_variables_end,
                                                          double a_argument)
    : variables_begin_(a_variables_begin), variables_end_(a_variables_end), argument_(a_argument)
{
}

size_t PolynomialThirdDerivativeAt ::variablesBegin() const { return variables_begin_; }

size_t PolynomialThirdDerivativeAt ::variablesEnd() const { return variables_end_; }

double PolynomialThirdDerivativeAt ::getCoefficient(size_t a_variable) const
{
    size_t degree = a_variable - variables_begin_;
    double result = 1.;
    for (size_t i = 3; i < degree; i++) {
        result *= argument_;
    }
    result *= (double)degree * (double)(degree - 1) * (double)(degree - 2);
    return result;
}

double PolynomialThirdDerivativeAt ::getConstant() const { return 0.; }

///////////////////////////////////////////////////////////////////////////////
// class Polynomial                                                          //
///////////////////////////////////////////////////////////////////////////////

Polynomial ::Polynomial(CoefficientSet *a_coefficient_set, size_t a_variables_begin, size_t a_variables_end)
    : coefficient_set_(a_coefficient_set), variables_begin_(a_variables_begin), variables_end_(a_variables_end)
{
}

Polynomial ::Polynomial() : coefficient_set_(nullptr), variables_begin_(-1), variables_end_(-1) {}

size_t Polynomial ::variablesBegin() const { return variables_begin_; }

size_t Polynomial ::variablesEnd() const { return variables_end_; }

size_t Polynomial ::degree() const { return variables_end_ - variables_begin_ - 1; }

PolynomialValueAt Polynomial ::valueAt(double a_argument) const
{
    return PolynomialValueAt(variables_begin_, variables_end_, a_argument);
}

PolynomialDerivativeAt Polynomial ::derivativeAt(double a_argument) const
{
    return PolynomialDerivativeAt(variables_begin_, variables_end_, a_argument);
}

PolynomialSecondDerivativeAt Polynomial ::secondDerivativeAt(double a_argument) const
{
    return PolynomialSecondDerivativeAt(variables_begin_, variables_end_, a_argument);
}

PolynomialThirdDerivativeAt Polynomial ::thirdDerivativeAt(double a_argument) const
{
    return PolynomialThirdDerivativeAt(variables_begin_, variables_end_, a_argument);
}

///////////////////////////////////////////////////////////////////////////////
// class CoefficientSet                                                       //
///////////////////////////////////////////////////////////////////////////////

size_t CoefficientSet::variablesCount() const { return variables_count_; }

Polynomial CoefficientSet::addPolynomial(int a_degree)
{
    size_t variables_begin = variables_count_;
    variables_count_ += a_degree + 1;
    size_t variables_end = variables_count_;
    Polynomial result(this, variables_begin, variables_end);
    return result;
}

///////////////////////////////////////////////////////////////////////////////
// class EquationSet                                                         //
///////////////////////////////////////////////////////////////////////////////

EquationSet ::EquationSet(const CoefficientSet &coefficient_set)
    : dimension_(coefficient_set.variablesCount()), equations_(0)
{
    linear_.resize(dimension_, dimension_);
    constant_.resize(dimension_, 1);
    results_.resize(1, dimension_);
}

void EquationSet ::init(const CoefficientSet &coefficient_set)
{
    dimension_ = coefficient_set.variablesCount();
    equations_ = 0;
    linear_.resize(dimension_, dimension_);
    constant_.resize(dimension_, 1);
    results_.resize(1, dimension_);
}

void EquationSet ::clear() { equations_ = 0; }

void EquationSet ::solve()
{
    assert(equations_ == dimension_);
    // std::cout << linear_ << std::endl;
    // std::cout << constant_ << std::endl;

    results_ = linear_.colPivHouseholderQr().solve(constant_);

    // std::cout << results_ << std::endl;
}

double EquationSet ::getCoefficient(const Polynomial &a_polynomial, size_t a_power) const
{
    return results_[a_polynomial.variablesBegin() + a_power];
}

Degree3Polynomial EquationSet ::buildDegree3Polynomial(const Polynomial &a_polynomial) const
{
    double a = 0.;
    double b = 0.;
    double c = 0.;
    double d = 0.;
    size_t degree = a_polynomial.degree();
    if (degree >= 3) {
        d = getCoefficient(a_polynomial, 3);
    }
    if (degree >= 2) {
        c = getCoefficient(a_polynomial, 2);
    }
    if (degree >= 1) {
        b = getCoefficient(a_polynomial, 1);
    }
    a = getCoefficient(a_polynomial, 0);
    return Degree3Polynomial(a, b, c, d);
}

Degree5Polynomial EquationSet ::buildDegree5Polynomial(const Polynomial &a_polynomial) const
{
    double a = 0.;
    double b = 0.;
    double c = 0.;
    double d = 0.;
    double e = 0.;
    double f = 0.;
    size_t degree = a_polynomial.degree();
    if (degree >= 5) {
        f = getCoefficient(a_polynomial, 5);
    }
    if (degree >= 4) {
        e = getCoefficient(a_polynomial, 4);
    }
    if (degree >= 3) {
        d = getCoefficient(a_polynomial, 3);
    }
    if (degree >= 2) {
        c = getCoefficient(a_polynomial, 2);
    }
    if (degree >= 1) {
        b = getCoefficient(a_polynomial, 1);
    }
    a = getCoefficient(a_polynomial, 0);
    return Degree5Polynomial(a, b, c, d, e, f);
}

Degree6Polynomial EquationSet ::buildDegree6Polynomial(const Polynomial &a_polynomial) const
{
    double a = 0.;
    double b = 0.;
    double c = 0.;
    double d = 0.;
    double e = 0.;
    double f = 0.;
    double g = 0.;
    size_t degree = a_polynomial.degree();
    if (degree >= 6) {
        g = getCoefficient(a_polynomial, 6);
    }
    if (degree >= 5) {
        f = getCoefficient(a_polynomial, 5);
    }
    if (degree >= 4) {
        e = getCoefficient(a_polynomial, 4);
    }
    if (degree >= 3) {
        d = getCoefficient(a_polynomial, 3);
    }
    if (degree >= 2) {
        c = getCoefficient(a_polynomial, 2);
    }
    if (degree >= 1) {
        b = getCoefficient(a_polynomial, 1);
    }
    a = getCoefficient(a_polynomial, 0);
    return Degree6Polynomial(a, b, c, d, e, f, g);
}

Degree7Polynomial EquationSet ::buildDegree7Polynomial(const Polynomial &a_polynomial) const
{
    double a = 0.;
    double b = 0.;
    double c = 0.;
    double d = 0.;
    double e = 0.;
    double f = 0.;
    double g = 0.;
    double h = 0.;
    size_t degree = a_polynomial.degree();
    if (degree >= 7) {
        h = getCoefficient(a_polynomial, 7);
    }
    if (degree >= 6) {
        g = getCoefficient(a_polynomial, 6);
    }
    if (degree >= 5) {
        f = getCoefficient(a_polynomial, 5);
    }
    if (degree >= 4) {
        e = getCoefficient(a_polynomial, 4);
    }
    if (degree >= 3) {
        d = getCoefficient(a_polynomial, 3);
    }
    if (degree >= 2) {
        c = getCoefficient(a_polynomial, 2);
    }
    if (degree >= 1) {
        b = getCoefficient(a_polynomial, 1);
    }
    a = getCoefficient(a_polynomial, 0);
    return Degree7Polynomial(a, b, c, d, e, f, g, h);
}

} // namespace kswx_weaving_generator
