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

#include <kord/polynomials/polynomials.h>

#include <cmath>
#include <iomanip>
#include <sstream>
#include <tuple>
#include <utility>

namespace kswx_weaving_generator {

/// 0 == k0 + k1*x
inline double fnGeneralLinearEquation(double k0, double k1, double min, double max)
{
    if (k1 == 0) {
        if (k0 == 0) {
            return (min + max) / 2; // any value is good
        }

        return NAN;
    }

    double x = -k0 / k1;
    if (min <= x && x <= max)
        return x;
    return NAN;
}

/// 0 == k0 + k1*x + 1*x*x
inline double fnNormalizedQuadraticEquation(double k0, double k1, double min, double max)
{
    // a == 1
    double b = k1;
    double c = k0;
    double d = b * b - 4 * c;
    if (d >= -1e-8) {
        double s;
        if (d > 0) {
            s = std::sqrt(d);
        }
        else {
            s = 0;
        }
        double x1 = (-b + s) / 2;
        double x2 = (-b - s) / 2;
        if (min <= x1 && x1 <= max)
            return x1;
        if (min <= x2 && x2 <= max)
            return x2;
        return NAN;
    }

    return NAN;
}

/// 0 == k0 + k1*x + k2*x*x
inline double fnGeneralQuadraticEquation(double k0, double k1, double k2, double min, double max)
{
    if (k2 == 0) {
        return fnGeneralLinearEquation(k0, k1, min, max);
    }

    return fnNormalizedQuadraticEquation(k0 / k2, k1 / k2, min - 1E-14, max + 1E-14);
}

/// 0 == k0 + 0*k1 + k2*x*x + 0*x*x*x + 1*x*x*x*x
inline double fnNormalizedBiquadraticEquation(double k0, double k2, double min, double max)
{
    // substitute t = x*x, solve for t
    double b = k2;
    double c = k0;
    double d = b * b - 4 * c;
    if (d > -1E-10) {
        if (d < 0) {
            d = 0;
        }
        double s = std::sqrt(d);
        double t1 = (-b + s) / 2;
        if (t1 < 0 && t1 > -1E-10) {
            t1 = 0;
        }
        double t2 = (-b - s) / 2;
        if (t2 < 0 && t2 > -1E-10) {
            t2 = 0;
        }
        double x1 = std::sqrt(t1);
        double x2 = std::sqrt(t2);
        double x3 = -x1;
        double x4 = -x2;
        if (min <= x1 && x1 <= max)
            return x1;
        if (min <= x2 && x2 <= max)
            return x2;
        if (min <= x3 && x3 <= max)
            return x3;
        if (min <= x4 && x4 <= max)
            return x4;
        return NAN;
    }

    return NAN;
}

/// 0 == k0 + k1*x + 0*x*x + 1*x*x*x
inline double fnDepressedCubicEquation(double k0, double k1, double min, double max)
{
    double p = k1;
    double q = k0;
    double t = q * q / 4 + p * p * p / 27;
    if (t >= 0) {
        // one root
        double s = std::sqrt(t);
        double c1 = -q / 2 + s;
        double c2 = -q / 2 - s;
        double r1;
        double r2;
        // pow is defined only for positive numbers, cubic root is well defined also for negative
        if (c1 > 0) {
            r1 = pow(c1, 1.0 / 3);
        }
        else {
            r1 = -pow(-c1, 1.0 / 3);
        };
        if (c2 > 0) {
            r2 = pow(c2, 1.0 / 3);
        }
        else {
            r2 = -pow(-c2, 1.0 / 3);
        };
        double x1 = r1 + r2;
        if (min <= x1 && x1 <= max)
            return x1;
    }
    if (t < 1E-10) { // t==0 signals double root, we might have missed it due to rounding error if we were strict t <= 0
        // three roots
        for (int kk = 0; kk < 3; kk++) {
            double k = (double)kk;
            double aca = 3 * q / 2 / p * std::sqrt(-3 / p);
            if (aca > 1 && aca < 1 + 1E-5) {
                aca = 1;
            }
            if (aca < -1 && aca > -1 - 1E-5) {
                aca = -1;
            }
            double ca = std::acos(aca) / 3 - 2 * M_PI * k / 3;
            double x = 2 * std::sqrt(-p / 3) * std::cos(ca);
            if (min <= x && x <= max)
                return x;
        }
    }
    return NAN;
}

/// 0 == k0 + k1*x + k2*x*x + 1*x*x*x
inline double fnNormalizedCubicEquation(double k0, double k1, double k2, double min, double max)
{
    // k4 == 1
    double b = k2;
    double c = k1;
    double d = k0;
    // substitute  x = u - b/3 and solve for u
    // n4 = 1, n3 = 0, n2 = al, n1 = be, n0 = ga
    double p = c - b * b / 3;
    double q = 2 * b * b * b / 27 - b * c / 3 + d;
    double result = fnDepressedCubicEquation(q, p, min + b / 3, max + b / 3);
    return result - b / 3;
}

/// 0 == k0 + k1*x + k2*x*x + k3*x*x*x
inline double fnGeneralCubicEquation(double k0, double k1, double k2, double k3, double min, double max)
{
    if (k3 == 0) {
        return fnGeneralQuadraticEquation(k0, k1, k2, min, max);
    }

    return fnNormalizedCubicEquation(k0 / k3, k1 / k3, k2 / k3, min - 1E-14, max + 1E-14);
}

/// 0 == k0 + k1*x + k2*x*x + 0*x*x*x + 1*x*x*x*x
inline double fnDepressedQuarticEquation(double k0, double k1, double k2, double min, double max)
{
    double al = k2;
    double be = k1;
    double ga = k0;
    if (std::abs(be) < 1E-12) {
        return fnNormalizedBiquadraticEquation(ga, al, min, max);
    }
    else if (std::abs(ga) < 1E-12) {
        double x1 = 0;
        if (min <= x1 && x1 <= max)
            return x1;
        return fnDepressedCubicEquation(be, al, min, max);
    }

    // derive to form (x*x + al + y)^2 = (al + 2*y)*x*x - be*x + (y*y + 2*y*al + al*al - ga)
    // see wikipedia, find such y that the right side can be transformed to (...*x + ...)^2
    // then (al + 2*y)*x*x - be*x + (y*y + 2*y*al + al*al - ga) = (l1*x + l2)^2
    double y = fnNormalizedCubicEquation(al * al * al / 2 - al * ga / 2 - be * be / 8,
                                         2 * al * al - ga,
                                         2.5 * al,
                                         -INFINITY,
                                         +INFINITY);
    double m1 = al + 2 * y;
    if (m1 < 0 && m1 > -1e-10) {
        m1 = 0; // we can get here by rounding errors
    }
    double l1 = std::sqrt(m1);
    double m2 = y * y + 2 * y * al + al * al - ga;
    if (m2 < 0 && m2 > -1e-10) {
        m2 = 0; // we can get here by rounding errors
    }
    double l2 = std::sqrt(m2);
    if (be >
        0) { // the other square root is needed, so -be would be 2*l1*l2 and the replacement by right side would work
        l2 = -l2;
    }
    // considering this equation again (x*x + al + y)^2 = (al + 2*y)*x*x - be*x + (y*y + 2*y*al + al*al - ga)
    // if is transformed to (x*x + al + y)^2 = (l1*x + l2)^2, we will square root both sides in two variants
    // a^2 = b^2 means either a = b or a = -b. We will pursue both cases, it is quadratic equation in both cases.
    // the first case: x*x + al + y = l1*x + l2
    double x1 = fnNormalizedQuadraticEquation(al + y + l2, +l1, min, max);
    if (!std::isnan(x1)) {
        return x1;
    }
    // the second case: x*x + al + y = -l1*x - l2
    double x2 = fnNormalizedQuadraticEquation(al + y - l2, -l1, min, max);
    if (!std::isnan(x2)) {
        return x2;
    }
    return NAN;
}

/// 0 == k0 + k1*x + k2*x*x + k3*x*x*x + 1*x*x*x*x
inline double fnNormalizedQuarticEquation(double k0, double k1, double k2, double k3, double min, double max)
{
    // k4 == 1
    double b = k3;
    double c = k2;
    double d = k1;
    double e = k0;
    // substitute  x = u - b/4/a and solve for u
    // n4 = 1, n3 = 0, n2 = al, n1 = be, n0 = ga
    double al = -3 * b * b / 8 + c;
    double be = b * b * b / 8 - b * c / 2 + d;
    double ga = -3 * b * b * b * b / 256 + c * b * b / 16 - b * d / 4 + e;
    return fnDepressedQuarticEquation(ga, be, al, min + b / 4, max + b / 4) - b / 4;
}

/// 0 == k0 + k1*x + k2*x*x + k3*x*x*x + k4*x*x*x*x
inline double fnGeneralQuarticEquation(double k0, double k1, double k2, double k3, double k4, double min, double max)
{
    if (k0 == 0 && min <= 0 && 0 <= max) {
        return 0;
    }
    if (k4 == 0) {
        return fnGeneralCubicEquation(k0, k1, k2, k3, min, max);
    }

    return fnNormalizedQuarticEquation(k0 / k4, k1 / k4, k2 / k4, k3 / k4, min, max);
}

inline double fnGeneralQuarticEquationNotNan(
    double k0, double k1, double k2, double k3, double k4, double min, double max, bool a_log_if_nan)
{
    return fnGeneralQuarticEquation(k0, k1, k2, k3, k4, min, max);
}

///////////////////////////////////////////////////////////////////////////////
// class Degree1Polynomial                                                   //
///////////////////////////////////////////////////////////////////////////////

Degree1Polynomial ::Degree1Polynomial() : a_(NAN), b_(NAN) {}

Degree1Polynomial ::Degree1Polynomial(double a_a, double a_b) : a_(a_a), b_(a_b) {}

double Degree1Polynomial ::root() const
{
    if (std::abs(b_) < 1e-10) {
        // actually constant
        if (std::abs(a_) < 1e-10) {
            return 0.; // any number is a root
        }

        return NAN; // no roots
    }

    // actually linear
    return -a_ / b_;
}

[[maybe_unused]] std::string Degree1Polynomial ::toString() const
{
    std::stringstream ss;
    ss << std::setprecision(15) << std::setw(18);
    ss << "Degree1Polynomial{ " << (std::abs(a_) < 1e-14 ? 0. : a_) << " + " << (std::abs(b_) < 1e-14 ? 0. : b_)
       << " *x }";
    return ss.str();
}

///////////////////////////////////////////////////////////////////////////////
// class Degree2Polynomial                                                   //
///////////////////////////////////////////////////////////////////////////////

Degree2Polynomial ::Degree2Polynomial() : a_(NAN), b_(NAN), c_(NAN) {}

Degree2Polynomial ::Degree2Polynomial(double a_a, double a_b, double a_c) : a_(a_a), b_(a_b), c_(a_c) {}

double Degree2Polynomial ::valueAt(double x) const { return a_ + b_ * x + c_ * x * x; }

double Degree2Polynomial ::derivativeAt(double x) const { return b_ + 2 * c_ * x; }

double Degree2Polynomial ::secondDerivativeAt([[maybe_unused]] double x) const { return 2 * c_; }

double Degree2Polynomial ::thirdDerivativeAt([[maybe_unused]] double x) const { return 0.; }

double Degree2Polynomial ::fourthDerivativeAt([[maybe_unused]] double x) const { return 0.; }

void Degree2Polynomial ::yShift(double a_shift) { a_ += a_shift; }

void Degree2Polynomial ::yStretch(double a_stretch)
{
    a_ *= a_stretch;
    b_ *= a_stretch;
    c_ *= a_stretch;
}

void Degree2Polynomial ::xStretch(double a_stretch)
{
    a_ /= 1;
    b_ /= a_stretch;
    c_ /= a_stretch * a_stretch;
}

void Degree2Polynomial ::xShift(double a_shift)
{
    a_ = a_ + b_ * a_shift + c_ * a_shift * a_shift;
    b_ = b_ + 2 * c_ * a_shift;
}

double Degree2Polynomial ::equalsConstantAt(double y, double x_min, double x_max) const
{
    return fnGeneralQuadraticEquation(a_ - y, b_, c_, x_min, x_max);
}

double Degree2Polynomial ::maximumOnInterval(double a_xmin, double a_xmax) const
{
    double root = derivative().root();
    double result = valueAt(a_xmin);
    double candidate = valueAt(a_xmax);
    if (result < candidate) {
        result = candidate;
    }
    if (a_xmin <= root && root <= a_xmax) {
        candidate = valueAt(root);
        if (result < candidate) {
            result = candidate;
        }
    }
    return result;
}

double Degree2Polynomial ::minimumOnInterval(double a_xmin, double a_xmax) const
{
    double root = derivative().root();
    double result = valueAt(a_xmin);
    double candidate = valueAt(a_xmax);
    if (result > candidate) {
        result = candidate;
    }
    if (a_xmin <= root && root <= a_xmax) {
        candidate = valueAt(root);
        if (result > candidate) {
            result = candidate;
        }
    }
    return result;
}

std::pair<double, double> Degree2Polynomial ::roots() const
{
    if (std::abs(c_) < 1e-10) {
        if (std::abs(b_) < 1e-10) {
            // actually constant
            if (std::abs(a_) < 1e-10) {
                return std::make_pair(0., INFINITY); // any number is a root
            }

            return std::make_pair(NAN, NAN); // no roots
        }

        // actually linear
        return std::make_pair(-a_ / b_, NAN);
    }

    double d = b_ * b_ - 4 * a_ * c_;
    if (d >= 0) {
        double s = std::sqrt(d);
        double x1 = (-b_ + s) / 2 / c_;
        double x2 = (-b_ - s) / 2 / c_;
        return std::make_pair(x1, x2);
    }

    return std::make_pair(NAN, NAN);
}

Degree1Polynomial Degree2Polynomial ::derivative() const { return Degree1Polynomial(b_, 2 * c_); }

Degree3Polynomial Degree2Polynomial ::integrate() const { return Degree3Polynomial(0., a_, b_ / 2, c_ / 3); }

std::string Degree2Polynomial ::toString() const
{
    std::stringstream ss;
    ss << std::setprecision(15) << std::setw(18);
    ss << "Degree2Polynomial{ " << (std::abs(a_) < 1e-14 ? 0. : a_) << " + " << (std::abs(b_) < 1e-14 ? 0. : b_)
       << " *x+ " << (std::abs(c_) < 1e-14 ? 0. : c_) << " *x^2 }";
    return ss.str();
}

///////////////////////////////////////////////////////////////////////////////
// class Degree3Polynomial                                                   //
///////////////////////////////////////////////////////////////////////////////

Degree3Polynomial ::Degree3Polynomial() : a_(NAN), b_(NAN), c_(NAN), d_(NAN) {}

Degree3Polynomial ::Degree3Polynomial(double a_a, double a_b, double a_c, double a_d)
    : a_(a_a), b_(a_b), c_(a_c), d_(a_d)
{
}

double Degree3Polynomial ::valueAt(double x) const { return a_ + b_ * x + c_ * x * x + d_ * x * x * x; }

double Degree3Polynomial ::derivativeAt(double x) const { return b_ + 2 * c_ * x + 3 * d_ * x * x; }

double Degree3Polynomial ::secondDerivativeAt(double x) const { return 2 * c_ + 6 * d_ * x; }

double Degree3Polynomial ::thirdDerivativeAt([[maybe_unused]] double x) const { return 6 * d_; }

double Degree3Polynomial ::fourthDerivativeAt([[maybe_unused]] double x) const { return 0; }

Degree2Polynomial Degree3Polynomial ::derivative() const { return Degree2Polynomial(b_, 2 * c_, 3 * d_); }

double Degree3Polynomial ::maximumOnInterval(double a_xmin, double a_xmax) const
{
    double root1;
    double root2;
    std::tie(root1, root2) = derivative().roots();
    double result = valueAt(a_xmin);
    double candidate = valueAt(a_xmax);
    if (result < candidate) {
        result = candidate;
    }
    if (a_xmin <= root1 && root1 <= a_xmax) {
        candidate = valueAt(root1);
        if (result < candidate) {
            result = candidate;
        }
    }
    if (a_xmin <= root2 && root2 <= a_xmax) {
        candidate = valueAt(root2);
        if (result < candidate) {
            result = candidate;
        }
    }
    return result;
}

double Degree3Polynomial ::minimumOnInterval(double a_xmin, double a_xmax) const
{
    double root1;
    double root2;
    std::tie(root1, root2) = derivative().roots();
    double result = valueAt(a_xmin);
    double candidate = valueAt(a_xmax);
    if (result > candidate) {
        result = candidate;
    }
    if (a_xmin <= root1 && root1 <= a_xmax) {
        candidate = valueAt(root1);
        if (result > candidate) {
            result = candidate;
        }
    }
    if (a_xmin <= root2 && root2 <= a_xmax) {
        candidate = valueAt(root2);
        if (result > candidate) {
            result = candidate;
        }
    }
    return result;
}

void Degree3Polynomial ::yShift(double a_shift) { a_ += a_shift; }

void Degree3Polynomial ::yStretch(double a_stretch)
{
    a_ *= a_stretch;
    b_ *= a_stretch;
    c_ *= a_stretch;
    d_ *= a_stretch;
}

void Degree3Polynomial ::xStretch(double a_stretch)
{
    a_ /= 1;
    b_ /= a_stretch;
    c_ /= a_stretch * a_stretch;
    d_ /= a_stretch * a_stretch * a_stretch;
}

void Degree3Polynomial ::xShift(double a_shift)
{
    a_ = a_ + b_ * a_shift + c_ * a_shift * a_shift + d_ * a_shift * a_shift * a_shift;
    b_ = b_ + 2 * c_ * a_shift + 3 * d_ * a_shift * a_shift;
    c_ = c_ + 3 * d_ * a_shift;
}

double Degree3Polynomial ::equalsConstantAt(double y, double x_min, double x_max) const
{
    return fnGeneralCubicEquation(a_ - y, b_, c_, d_, x_min, x_max);
}

std::string Degree3Polynomial ::toString() const
{
    std::stringstream ss;
    ss << std::setprecision(15) << std::setw(18);
    ss << "Degree3Polynomial{ " << (std::abs(a_) < 1e-14 ? 0. : a_) << " + " << (std::abs(b_) < 1e-14 ? 0. : b_)
       << " *x+ " << (std::abs(c_) < 1e-14 ? 0. : c_) << " *x^2+ " << (std::abs(d_) < 1e-14 ? 0. : d_) << " *x^3 }";
    return ss.str();
}

Degree4Polynomial Degree3Polynomial ::integrate() const { return Degree4Polynomial(0., a_, b_ / 2, c_ / 3, d_ / 4); }

///////////////////////////////////////////////////////////////////////////////
// class Degree4Polynomial                                                   //
///////////////////////////////////////////////////////////////////////////////

Degree4Polynomial ::Degree4Polynomial() : a_(NAN), b_(NAN), c_(NAN), d_(NAN), e_(NAN) {}

Degree4Polynomial ::Degree4Polynomial(double a_a, double a_b, double a_c, double a_d, double a_e)
    : a_(a_a), b_(a_b), c_(a_c), d_(a_d), e_(a_e)
{
}

double Degree4Polynomial ::valueAt(double x) const
{
    return a_ + b_ * x + c_ * x * x + d_ * x * x * x + e_ * x * x * x * x;
}

double Degree4Polynomial ::derivativeAt(double x) const { return b_ + 2 * c_ * x + 3 * d_ * x * x + 4 * e_ * x * x * x; }

double Degree4Polynomial ::secondDerivativeAt(double x) const { return 2 * c_ + 6 * d_ * x + 12 * e_ * x * x; }

double Degree4Polynomial ::thirdDerivativeAt(double x) const { return 6 * d_ + 24 * e_ * x; }

double Degree4Polynomial ::fourthDerivativeAt([[maybe_unused]] double x) const { return 24 * e_; }

Degree3Polynomial Degree4Polynomial ::derivative() const { return Degree3Polynomial(b_, 2 * c_, 3 * d_, 4 * e_); }

Degree5Polynomial Degree4Polynomial ::extendDegree() const { return Degree5Polynomial(a_, b_, c_, d_, e_, 0.); }

double Degree4Polynomial ::maximumOnInterval(double a_xmin, double a_xmax) const
{
    double result = valueAt(a_xmin);
    double candidate = valueAt(a_xmax);
    if (result < candidate) {
        result = candidate;
    }

    // for finding extremes, we need to find roots of derivative().
    // Degree3Polynomial has equalsConstantAt method, which returns any
    // root on interval. In order to know what intervals to ask for, we
    // will compute inflex points first, those will chunk a_xmin..a_xmax
    // into intervals with one extreme point each.
    Degree3Polynomial derivative = this->derivative();
    Degree2Polynomial second_der = derivative.derivative();
    double inflex1;
    double inflex2;
    std::tie(inflex1, inflex2) = second_der.roots();
    if (std::isnan(inflex1) && std::isnan(inflex2)) {
        // std::isnan(inflex1) <=> std::isnan(inflex2), because those are
        // roots of quadratic equation
        // no inflex point means there is only one extreme
        double extreme = derivative.equalsConstantAt(0., -INFINITY, INFINITY);
        if (a_xmin < extreme && extreme < a_xmax) {
            double candidate = valueAt(extreme);
            if (result < candidate) {
                result = candidate;
            }
        }
    }
    else if (!std::isnan(inflex1) && std::isnan(inflex2)) {
        double extreme1 = derivative.equalsConstantAt(0., -INFINITY, inflex1);
        double extreme2 = derivative.equalsConstantAt(0., inflex1, INFINITY);
        if (a_xmin < extreme1 && extreme1 < a_xmax) {
            double candidate = valueAt(extreme1);
            if (result < candidate) {
                result = candidate;
            }
        }
        if (a_xmin < extreme2 && extreme2 < a_xmax) {
            double candidate = valueAt(extreme2);
            if (result < candidate) {
                result = candidate;
            }
        }
    }
    else {
        double extreme1 = derivative.equalsConstantAt(0., -INFINITY, inflex1);
        double extreme2 = derivative.equalsConstantAt(0., inflex1, inflex2);
        double extreme3 = derivative.equalsConstantAt(0., inflex2, INFINITY);
        if (a_xmin < extreme1 && extreme1 < a_xmax) {
            double candidate = valueAt(extreme1);
            if (result < candidate) {
                result = candidate;
            }
        }
        if (a_xmin < extreme2 && extreme2 < a_xmax) {
            double candidate = valueAt(extreme2);
            if (result < candidate) {
                result = candidate;
            }
        }
        if (a_xmin < extreme3 && extreme3 < a_xmax) {
            double candidate = valueAt(extreme3);
            if (result < candidate) {
                result = candidate;
            }
        }
    }
    return result;
}

double Degree4Polynomial ::minimumOnInterval(double a_xmin, double a_xmax) const
{
    Degree4Polynomial inverted(*this);
    inverted.yStretch(-1);
    return -inverted.maximumOnInterval(a_xmin, a_xmax);
}

void Degree4Polynomial ::yShift(double a_shift) { a_ += a_shift; }

void Degree4Polynomial ::xShift(double a_shift)
{
    a_ = a_ + b_ * a_shift + c_ * a_shift * a_shift + d_ * a_shift * a_shift * a_shift +
         e_ * a_shift * a_shift * a_shift * a_shift;
    b_ = b_ + 2 * c_ * a_shift + 3 * d_ * a_shift * a_shift + 4 * e_ * a_shift * a_shift * a_shift;
    c_ = c_ + 3 * d_ * a_shift + 6 * e_ * a_shift * a_shift;
    d_ = d_ + 4 * e_ * a_shift;
}

void Degree4Polynomial ::xStretch(double a_stretch)
{
    a_ /= 1;
    b_ /= a_stretch;
    c_ /= a_stretch * a_stretch;
    d_ /= a_stretch * a_stretch * a_stretch;
    e_ /= a_stretch * a_stretch * a_stretch * a_stretch;
}

void Degree4Polynomial ::yStretch(double a_stretch)
{
    a_ *= a_stretch;
    b_ *= a_stretch;
    c_ *= a_stretch;
    d_ *= a_stretch;
    e_ *= a_stretch;
}

double Degree4Polynomial ::equalsConstantAt(double y, double x_min, double x_max, bool a_log_if_nan) const
{
    return fnGeneralQuarticEquationNotNan(a_ - y, b_, c_, d_, e_, x_min, x_max, a_log_if_nan);
}

std::string Degree4Polynomial ::toString() const
{
    std::stringstream ss;
    ss << std::setprecision(15) << std::setw(18);
    ss << "Degree4Polynomial{ " << (std::abs(a_) < 1e-14 ? 0. : a_) << " + " << (std::abs(b_) < 1e-14 ? 0. : b_)
       << " *x+ " << (std::abs(c_) < 1e-14 ? 0. : c_) << " *x^2+ " << (std::abs(d_) < 1e-14 ? 0. : d_) << " *x^3+ "
       << (std::abs(e_) < 1e-14 ? 0. : e_) << " *x^4 }";
    return ss.str();
}

///////////////////////////////////////////////////////////////////////////////
// class Degree5Polynomial                                                   //
///////////////////////////////////////////////////////////////////////////////

Degree5Polynomial ::Degree5Polynomial() : a_(NAN), b_(NAN), c_(NAN), d_(NAN), e_(NAN), f_(NAN) {}

Degree5Polynomial ::Degree5Polynomial(double a_a, double a_b, double a_c, double a_d, double a_e, double a_f)
    : a_(a_a), b_(a_b), c_(a_c), d_(a_d), e_(a_e), f_(a_f)
{
}

double Degree5Polynomial ::valueAt(double x) const
{
    return a_ + b_ * x + c_ * x * x + d_ * x * x * x + e_ * x * x * x * x + f_ * x * x * x * x * x;
}

double Degree5Polynomial ::derivativeAt(double x) const
{
    return b_ + 2 * c_ * x + 3 * d_ * x * x + 4 * e_ * x * x * x + 5 * f_ * x * x * x * x;
}

double Degree5Polynomial ::secondDerivativeAt(double x) const
{
    return 2 * c_ + 6 * d_ * x + 12 * e_ * x * x + 20 * f_ * x * x * x;
}

double Degree5Polynomial ::thirdDerivativeAt(double x) const { return 6 * d_ + 24 * e_ * x + 60 * f_ * x * x; }

double Degree5Polynomial ::fourthDerivativeAt(double x) const { return 24 * e_ + 120 * f_ * x; }

Degree4Polynomial Degree5Polynomial ::derivative() const
{
    return Degree4Polynomial(b_, 2 * c_, 3 * d_, 4 * e_, 5 * f_);
}

Degree6Polynomial Degree5Polynomial ::integrate(double x_0) const
{
    return Degree6Polynomial(a_ / 6, b_ / 5, c_ / 4, d_ / 3, e_ / 2, f_, x_0);
}

void Degree5Polynomial ::xShift(double a_shift)
{
    a_ = a_ + b_ * a_shift + c_ * a_shift * a_shift + d_ * a_shift * a_shift * a_shift +
         e_ * a_shift * a_shift * a_shift * a_shift + f_ * a_shift * a_shift * a_shift * a_shift * a_shift;
    b_ = b_ + 2 * c_ * a_shift + 3 * d_ * a_shift * a_shift + 4 * e_ * a_shift * a_shift * a_shift +
         5 * f_ * a_shift * a_shift * a_shift * a_shift;
    c_ = c_ + 3 * d_ * a_shift + 6 * e_ * a_shift * a_shift + 10 * f_ * a_shift * a_shift * a_shift;
    d_ = d_ + 4 * e_ * a_shift + 10 * f_ * a_shift * a_shift;
    e_ = e_ + 5 * f_ * a_shift;
}

void Degree5Polynomial ::yShift(double a_shift) { a_ += a_shift; }

void Degree5Polynomial ::xStretch(double a_stretch)
{
    a_ /= 1;
    b_ /= a_stretch;
    c_ /= a_stretch * a_stretch;
    d_ /= a_stretch * a_stretch * a_stretch;
    e_ /= a_stretch * a_stretch * a_stretch * a_stretch;
    f_ /= a_stretch * a_stretch * a_stretch * a_stretch * a_stretch;
}

void Degree5Polynomial ::yStretch(double a_stretch)
{
    a_ *= a_stretch;
    b_ *= a_stretch;
    c_ *= a_stretch;
    d_ *= a_stretch;
    e_ *= a_stretch;
    f_ *= a_stretch;
}

std::string Degree5Polynomial ::toString() const
{
    std::stringstream ss;
    ss << std::setprecision(15) << std::setw(18);
    ss << "Degree5Polynomial{ " << (std::abs(a_) < 1e-14 ? 0. : a_) << " + " << (std::abs(b_) < 1e-14 ? 0. : b_)
       << " *x+ " << (std::abs(c_) < 1e-14 ? 0. : c_) << " *x^2+ " << (std::abs(d_) < 1e-14 ? 0. : d_) << " *x^3+ "
       << (std::abs(e_) < 1e-14 ? 0. : e_) << " *x^4+ " << (std::abs(f_) < 1e-14 ? 0. : f_) << " *x^5 }";
    return ss.str();
}

///////////////////////////////////////////////////////////////////////////////
// class Degree6Polynomial                                                   //
///////////////////////////////////////////////////////////////////////////////

Degree6Polynomial ::Degree6Polynomial() : a_(NAN), b_(NAN), c_(NAN), d_(NAN), e_(NAN), f_(NAN), g_(NAN) {}

Degree6Polynomial ::Degree6Polynomial(double a_a, double a_b, double a_c, double a_d, double a_e, double a_f, double a_g)
    : a_(a_a), b_(a_b), c_(a_c), d_(a_d), e_(a_e), f_(a_f), g_(a_g)
{
}

double Degree6Polynomial ::valueAt(double x) const
{
    return a_ + b_ * x + c_ * x * x + d_ * x * x * x + e_ * x * x * x * x + f_ * x * x * x * x * x +
           g_ * x * x * x * x * x * x;
}

double Degree6Polynomial ::derivativeAt(double x) const
{
    return b_ + 2 * c_ * x + 3 * d_ * x * x + 4 * e_ * x * x * x + 5 * f_ * x * x * x * x + 6 * g_ * x * x * x * x * x;
}

double Degree6Polynomial ::secondDerivativeAt(double x) const
{
    return 2 * c_ + 6 * d_ * x + 12 * e_ * x * x + 20 * f_ * x * x * x + 30 * g_ * x * x * x * x;
}

double Degree6Polynomial ::thirdDerivativeAt(double x) const
{
    return 6 * d_ + 24 * e_ * x + 60 * f_ * x * x + 120 * g_ * x * x * x;
}

double Degree6Polynomial ::fourthDerivativeAt(double x) const { return 24 * e_ + 120 * f_ * x + 360 * g_ * x * x; }

std::string Degree6Polynomial ::toString() const
{
    std::stringstream ss;
    ss << std::setprecision(15) << std::setw(18);
    ss << "Degree6Polynomial{ " << (std::abs(a_) < 1e-14 ? 0. : a_) << " + " << (std::abs(b_) < 1e-14 ? 0. : b_)
       << " *x+ " << (std::abs(c_) < 1e-14 ? 0. : c_) << " *x^2+ " << (std::abs(d_) < 1e-14 ? 0. : d_) << " *x^3+ "
       << (std::abs(e_) < 1e-14 ? 0. : e_) << " *x^4+ " << (std::abs(f_) < 1e-14 ? 0. : f_) << " *x^5+ "
       << (std::abs(g_) < 1e-14 ? 0. : g_) << " *x^6 }";
    return ss.str();
}

///////////////////////////////////////////////////////////////////////////////
// class Degree7Polynomial                                                   //
///////////////////////////////////////////////////////////////////////////////

Degree7Polynomial ::Degree7Polynomial() : a_(NAN), b_(NAN), c_(NAN), d_(NAN), e_(NAN), f_(NAN), g_(NAN), h_(NAN) {}

Degree7Polynomial ::Degree7Polynomial(
    double a_a, double a_b, double a_c, double a_d, double a_e, double a_f, double a_g, double a_h)
    : a_(a_a), b_(a_b), c_(a_c), d_(a_d), e_(a_e), f_(a_f), g_(a_g), h_(a_h)
{
}

double Degree7Polynomial ::valueAt(double x) const
{
    return a_ + b_ * x + c_ * x * x + d_ * x * x * x + e_ * x * x * x * x + f_ * x * x * x * x * x +
           g_ * x * x * x * x * x * x + h_ * x * x * x * x * x * x * x;
}

double Degree7Polynomial ::derivativeAt(double x) const
{
    return b_ + 2 * c_ * x + 3 * d_ * x * x + 4 * e_ * x * x * x + 5 * f_ * x * x * x * x + 6 * g_ * x * x * x * x * x +
           7 * h_ * x * x * x * x * x * x;
}

double Degree7Polynomial ::secondDerivativeAt(double x) const
{
    return 2 * c_ + 6 * d_ * x + 12 * e_ * x * x + 20 * f_ * x * x * x + 30 * g_ * x * x * x * x +
           42 * h_ * x * x * x * x * x;
}

double Degree7Polynomial ::thirdDerivativeAt(double x) const
{
    return 6 * d_ + 24 * e_ * x + 60 * f_ * x * x + 120 * g_ * x * x * x + 210 * h_ * x * x * x * x;
}

double Degree7Polynomial ::fourthDerivativeAt(double x) const
{
    return 24 * e_ + 120 * f_ * x + 360 * g_ * x * x + 840 * h_ * x * x * x;
}

void Degree7Polynomial ::xShift(double a_shift)
{
    double s0 = 1.;
    double s1 = a_shift;
    double s2 = a_shift * a_shift;
    double s3 = a_shift * a_shift * a_shift;
    double s4 = a_shift * a_shift * a_shift * a_shift;
    double s5 = a_shift * a_shift * a_shift * a_shift * a_shift;
    double s6 = a_shift * a_shift * a_shift * a_shift * a_shift * a_shift;
    double s7 = a_shift * a_shift * a_shift * a_shift * a_shift * a_shift * a_shift;
    a_ = 1 * a_ * s0 + 1 * b_ * s1 + 1 * c_ * s2 + 1 * d_ * s3 + 1 * e_ * s4 + 1 * f_ * s5 + 1 * g_ * s6 + 1 * h_ * s7;
    b_ = 1 * b_ * s0 + 2 * c_ * s1 + 3 * d_ * s2 + 4 * e_ * s3 + 5 * f_ * s4 + 6 * g_ * s5 + 7 * h_ * s6;
    c_ = 1 * c_ * s0 + 3 * d_ * s1 + 6 * e_ * s2 + 10 * f_ * s3 + 15 * g_ * s4 + 21 * h_ * s5;
    d_ = 1 * d_ * s0 + 4 * e_ * s1 + 10 * f_ * s2 + 20 * g_ * s3 + 35 * h_ * s4;
    e_ = 1 * e_ * s0 + 5 * f_ * s1 + 15 * g_ * s2 + 35 * h_ * s3;
    f_ = 1 * f_ * s0 + 6 * g_ * s1 + 21 * h_ * s2;
    g_ = 1 * g_ * s0 + 7 * h_ * s1;
    h_ = 1 * h_ * s0;
}

void Degree7Polynomial ::yShift(double a_shift) { a_ += a_shift; }

void Degree7Polynomial ::xStretch(double a_stretch)
{
    a_ /= 1;
    b_ /= a_stretch;
    c_ /= a_stretch * a_stretch;
    d_ /= a_stretch * a_stretch * a_stretch;
    e_ /= a_stretch * a_stretch * a_stretch * a_stretch;
    f_ /= a_stretch * a_stretch * a_stretch * a_stretch * a_stretch;
    g_ /= a_stretch * a_stretch * a_stretch * a_stretch * a_stretch * a_stretch;
    h_ /= a_stretch * a_stretch * a_stretch * a_stretch * a_stretch * a_stretch * a_stretch;
}

void Degree7Polynomial ::yStretch(double a_stretch)
{
    a_ *= a_stretch;
    b_ *= a_stretch;
    c_ *= a_stretch;
    d_ *= a_stretch;
    e_ *= a_stretch;
    f_ *= a_stretch;
    g_ *= a_stretch;
    h_ *= a_stretch;
}

std::string Degree7Polynomial ::toString() const
{
    std::stringstream ss;
    ss << std::setprecision(15) << std::setw(18);
    ss << "Degree7Polynomial{ " << (std::abs(a_) < 1e-14 ? 0. : a_) << " + " << (std::abs(b_) < 1e-14 ? 0. : b_)
       << " *x+ " << (std::abs(c_) < 1e-14 ? 0. : c_) << " *x^2+ " << (std::abs(d_) < 1e-14 ? 0. : d_) << " *x^3+ "
       << (std::abs(e_) < 1e-14 ? 0. : e_) << " *x^4+ " << (std::abs(f_) < 1e-14 ? 0. : f_) << " *x^5+ "
       << (std::abs(g_) < 1e-14 ? 0. : g_) << " *x^6+ " << (std::abs(h_) < 1e-14 ? 0. : h_) << " *x^7 }";
    return ss.str();
}

}; // namespace kswx_weaving_generator
