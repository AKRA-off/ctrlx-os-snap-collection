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

#ifndef WEAVING_GENERATOR_SPLINE_COMPUTATION_H
#define WEAVING_GENERATOR_SPLINE_COMPUTATION_H

#include <kord/polynomials/polynomials.h>
#include <kord/polynomials/spline_equations.h>

#include <cstdlib>
#include <vector>

namespace kswx_weaving_generator {

/**
 * \brief Class for computing Spline of 6 polynomials with C2 continuity
 *
 * The spline is defined by values in 6 points, initial first, second and
 * third derivative and final first and second derivative.
 *
 * Construction of this class requires an allocation. It is recomended to
 * construct it and then use it multiple times
 */
class SplineC2S6 {
public:
    SplineC2S6();

    void setXs(double a_x0, double a_x1, double a_x2, double a_x3, double a_x4, double a_x5, double a_x6);
    void setYs(double a_y0, double a_y1, double a_y2, double a_y3, double a_y4, double a_y5, double a_y6);
    /**
     * \brief sets first and second derivative at point x0
     */
    void setStartConds(double a_d0, double a_s0);
    /**
     * \brief sets first and second derivative at point x6
     *
     * usually set to 0 (stopping the motion)
     */
    void setEndConds(double a_d6, double a_s6);
    /**
     * \brief performs spline computation for inputs set by set_* methods
     * \param[out] results pushes computed polynomials to this vector
     *
     * methods setXs, setYs, setStartConds and setEndConds must be
     * called before calling this method
     */
    void computeAndSaveTo(std::vector<Degree5Polynomial> &results);

private:
    CoefficientSet s3c_;
    Polynomial s3p0_;
    Polynomial s3p1_;
    Polynomial s3p2_;
    Polynomial s3p3_;
    Polynomial s3p4_;
    Polynomial s3p5_;
    EquationSet s3e_;
    double y0_;
    double y1_;
    double y2_;
    double y3_;
    double y4_;
    double y5_;
    double y6_;
    double h0_;
    double h1_;
    double h2_;
    double h3_;
    double h4_;
    double h5_;
    double d0_;
    double d6_;
    double s0_;
    double s6_;
};

/**
 * \brief Class for computing Spline of 5 polynomials with C2 continuity
 *
 * The spline is defined by values in 5 points, initial first, second and
 * third derivative and final first and second derivative.
 *
 * Construction of this class requires an allocation. It is recomended to
 * construct it and then use it multiple times
 */
class SplineC2S5 {
public:
    SplineC2S5();

    void setXs(double a_x0, double a_x1, double a_x2, double a_x3, double a_x4, double a_x5);
    void setYs(double a_y0, double a_y1, double a_y2, double a_y3, double a_y4, double a_y5);
    /**
     * \brief sets first and second derivative at point x0
     */
    void setStartConds(double a_d0, double a_s0);
    /**
     * \brief sets first and second derivative at point x3
     *
     * usually set to 0 (stopping the motion)
     */
    void setEndConds(double a_d5, double a_s5);
    /**
     * \brief performs spline computation for inputs set by set_* methods
     * \param[out] results pushes computed polynomials to this vector
     *
     * methods setXs, setYs, setStartConds and setEndConds must be
     * called before calling this method
     */
    void computeAndSaveTo(std::vector<Degree5Polynomial> &results);

private:
    CoefficientSet s3c_;
    Polynomial s3p0_;
    Polynomial s3p1_;
    Polynomial s3p2_;
    Polynomial s3p3_;
    Polynomial s3p4_;
    EquationSet s3e_;
    double y0_;
    double y1_;
    double y2_;
    double y3_;
    double y4_;
    double y5_;
    double h0_;
    double h1_;
    double h2_;
    double h3_;
    double h4_;
    double d0_;
    double d5_;
    double s0_;
    double s5_;
};

/**
 * \brief Class for computing Spline of 4 polynomials with C2 continuity
 *
 * The spline is defined by values in 5 points, initial first, second and
 * third derivative and final first and second derivative.
 *
 * Construction of this class requires an allocation. It is recomended to
 * construct it and then use it multiple times
 */
class SplineC2S4 {
public:
    SplineC2S4();

    void setXs(double a_x0, double a_x1, double a_x2, double a_x3, double a_x4);
    void setYs(double a_y0, double a_y1, double a_y2, double a_y3, double a_y4);
    /**
     * \brief sets first and second derivative at point x0
     */
    void setStartConds(double a_d0, double a_s0);
    /**
     * \brief sets first and second derivative at point x3
     *
     * usually set to 0 (stopping the motion)
     */
    void setEndConds(double a_d4, double a_s4);
    /**
     * \brief performs spline computation for inputs set by set_* methods
     * \param[out] results pushes computed polynomials to this vector
     *
     * methods setXs, setYs, setStartConds and setEndConds must be
     * called before calling this method
     */
    void computeAndSaveTo(std::vector<Degree5Polynomial> &results);

private:
    CoefficientSet s3c_;
    Polynomial s3p0_;
    Polynomial s3p1_;
    Polynomial s3p2_;
    Polynomial s3p3_;
    EquationSet s3e_;
    double y0_;
    double y1_;
    double y2_;
    double y3_;
    double y4_;
    double h0_;
    double h1_;
    double h2_;
    double h3_;
    double d0_;
    double d4_;
    double s0_;
    double s4_;
};

/**
 * \brief Class for computing Spline of 3 polynomials with C2 continuity
 *
 * The spline is defined by values in 4 points and initial and final
 * first and second derivative
 *
 * Construction of this class requires an allocation. It is recomended to
 * construct it and then use it multiple times
 */
class SplineC2S3 {
public:
    SplineC2S3();

    void setXs(double a_x0, double a_x1, double a_x2, double a_x3);
    void setYs(double a_y0, double a_y1, double a_y2, double a_y3);
    /**
     * \brief sets first and second derivative at point x0
     */
    void setStartConds(double a_d0, double a_s0);
    /**
     * \brief sets first and second derivative at point x3
     */
    void setEndConds(double a_d3, double a_s3);
    /**
     * \brief performs spline computation for inputs set by set_* methods
     * \param[out] results pushes computed polynomials to this vector
     *
     * methods setXs, setYs, setStartConds and setEndConds must be
     * called before calling this method
     */
    void computeAndSaveTo(std::vector<Degree5Polynomial> &results);

private:
    CoefficientSet s3c_;
    Polynomial s3p0_;
    Polynomial s3p1_;
    Polynomial s3p2_;
    EquationSet s3e_;
    double y0_;
    double y1_;
    double y2_;
    double y3_;
    double h0_;
    double h1_;
    double h2_;
    double d0_;
    double d3_;
    double s0_;
    double s3_;
};

/**
 * \brief Class for computing Spline of 2 polynomials with C2 continuity
 *
 * The spline is defined by values in 3 points and initial and final
 * first and second derivative
 *
 * Construction of this class requires an allocation. It is recomended to
 * construct it and then use it multiple times
 */
class SplineC2S2 {
public:
    SplineC2S2();

    void setXs(double a_x0, double a_x1, double a_x2);
    void setYs(double a_y0, double a_y1, double a_y2);
    /**
     * \brief sets first and second derivative at point x0
     */
    void setStartConds(double a_d0, double a_s0);
    /**
     * \brief sets first and second derivative at point x2
     *
     * usually set to 0 (stopping the motion)
     */
    void setEndConds(double a_d2, double a_s2);
    /**
     * \brief performs spline computation for inputs set by set_* methods
     * \param[out] results pushes computed polynomials to this vector
     *
     * methods setXs, setYs, setStartConds and setEndConds must be
     * called before calling this method
     */
    void computeAndSaveTo(std::vector<Degree5Polynomial> &results);

private:
    CoefficientSet s2c_;
    Polynomial s2p0_;
    Polynomial s2p1_;
    EquationSet s2e_;
    double y0_;
    double y1_;
    double y2_;
    double h0_;
    double h1_;
    double d0_;
    double d2_;
    double s0_;
    double s2_;
};

/**
 * \brief Class for computing Spline of 1 polynomials with C2 continuity
 *
 * The spline is defined by values in 2 points and initial and final
 * first and second derivative
 *
 * Construction of this class requires an allocation. It is recomended to
 * construct it and then use it multiple times
 */
class SplineC2S1 {
public:
    SplineC2S1();

    void setXs(double a_x0, double a_x1);
    void setYs(double a_y0, double a_y1);
    /**
     * \brief sets first and second derivative at point x0
     */
    void setStartConds(double a_d0, double a_s0);
    /**
     * \brief sets first and second derivative at point x2
     *
     * usually set to 0 (stopping the motion)
     */
    void setEndConds(double a_d1, double a_s1);
    /**
     * \brief performs spline computation for inputs set by set_* methods
     * \param[out] results pushes computed polynomials to this vector
     *
     * methods setXs, setYs, setStartConds and setEndConds must be
     * called before calling this method
     */
    void computeAndSaveTo(std::vector<Degree5Polynomial> &results);

private:
    CoefficientSet s1c_;
    Polynomial s1p0_;
    EquationSet s1e_;
    double y0_;
    double y1_;
    double h0_;
    double d0_;
    double d1_;
    double s0_;
    double s1_;
};

/**
 * \brief Class for computing Spline of 4 polynomials with C3 continuity
 *
 * The spline is defined by values in 5 points, initial first, second and
 * third derivative and final first and second derivative.
 *
 * Construction of this class requires an allocation. It is recomended to
 * construct it and then use it multiple times
 */
class SplineC2aS4 {
public:
    SplineC2aS4();

    void setXs(double a_x0, double a_x1, double a_x2, double a_x3, double a_x4);
    void setYs(double a_y0, double a_y1, double a_y2, double a_y3, double a_y4);
    /**
     * \brief sets first and second derivative at point x0
     */
    void setStartConds(double a_d0, double a_s0, double a_j0);
    /**
     * \brief sets first and second derivative at point x3
     *
     * usually set to 0 (stopping the motion)
     */
    void setEndConds(double a_d4, double a_s4);
    /**
     * \brief performs spline computation for inputs set by set_* methods
     * \param[out] results pushes computed polynomials to this vector
     *
     * methods setXs, setYs, setStartConds and setEndConds must be
     * called before calling this method
     */
    void computeAndSaveTo(std::vector<Degree6Polynomial> &results);

private:
    CoefficientSet s3c_;
    Polynomial s3p0_;
    Polynomial s3p1_;
    Polynomial s3p2_;
    Polynomial s3p3_;
    EquationSet s3e_;
    double y0_;
    double y1_;
    double y2_;
    double y3_;
    double y4_;
    double h0_;
    double h1_;
    double h2_;
    double h3_;
    double d0_;
    double d4_;
    double s0_;
    double s4_;
    double j0_;
};

/**
 * \brief Class for computing Spline of 3 polynomials with C2 continuity
 *
 * The spline is defined by values in 4 points and initial and final
 * first and second derivative
 *
 * Construction of this class requires an allocation. It is recomended to
 * construct it and then use it multiple times
 */
class SplineC2aS3 {
public:
    SplineC2aS3();

    void setXs(double a_x0, double a_x1, double a_x2, double a_x3);
    void setYs(double a_y0, double a_y1, double a_y2, double a_y3);
    /**
     * \brief sets first and second derivative at point x0
     */
    void setStartConds(double a_d0, double a_s0, double a_j0);
    /**
     * \brief sets first and second derivative at point x3
     */
    void setEndConds(double a_d3, double a_s3);
    /**
     * \brief performs spline computation for inputs set by set_* methods
     * \param[out] results pushes computed polynomials to this vector
     *
     * methods setXs, setYs, setStartConds and setEndConds must be
     * called before calling this method
     */
    void computeAndSaveTo(std::vector<Degree6Polynomial> &results);

private:
    CoefficientSet s3c_;
    Polynomial s3p0_;
    Polynomial s3p1_;
    Polynomial s3p2_;
    EquationSet s3e_;
    double y0_;
    double y1_;
    double y2_;
    double y3_;
    double h0_;
    double h1_;
    double h2_;
    double d0_;
    double d3_;
    double s0_;
    double s3_;
    double j0_;
};

/**
 * \brief Class for computing Spline of 2 polynomials with C2 continuity
 *
 * The spline is defined by values in 3 points and initial and final
 * first and second derivative
 *
 * Construction of this class requires an allocation. It is recomended to
 * construct it and then use it multiple times
 */
class SplineC2aS2 {
public:
    SplineC2aS2();

    void setXs(double a_x0, double a_x1, double a_x2);
    void setYs(double a_y0, double a_y1, double a_y2);
    /**
     * \brief sets first and second derivative at point x0
     */
    void setStartConds(double a_d0, double a_s0, double a_j0);
    /**
     * \brief sets first and second derivative at point x2
     *
     * usually set to 0 (stopping the motion)
     */
    void setEndConds(double a_d2, double a_s2);
    /**
     * \brief performs spline computation for inputs set by set_* methods
     * \param[out] results pushes computed polynomials to this vector
     *
     * methods setXs, setYs, setStartConds and setEndConds must be
     * called before calling this method
     */
    void computeAndSaveTo(std::vector<Degree6Polynomial> &results);

private:
    CoefficientSet s2c_;
    Polynomial s2p0_;
    Polynomial s2p1_;
    EquationSet s2e_;
    double y0_;
    double y1_;
    double y2_;
    double h0_;
    double h1_;
    double d0_;
    double d2_;
    double s0_;
    double s2_;
    double j0_;
};

/**
 * \brief Class for computing Spline of 1 polynomials with C2 continuity
 *
 * The spline is defined by values in 2 points and initial and final
 * first and second derivative
 *
 * Construction of this class requires an allocation. It is recomended to
 * construct it and then use it multiple times
 */
class SplineC2aS1 {
public:
    SplineC2aS1();

    void setXs(double a_x0, double a_x1);
    void setYs(double a_y0, double a_y1);
    /**
     * \brief sets first and second derivative at point x0
     */
    void setStartConds(double a_d0, double a_s0, double a_j0);
    /**
     * \brief sets first and second derivative at point x2
     *
     * usually set to 0 (stopping the motion)
     */
    void setEndConds(double a_d1, double a_s1);
    /**
     * \brief performs spline computation for inputs set by set_* methods
     * \param[out] results pushes computed polynomials to this vector
     *
     * methods setXs, setYs, setStartConds and setEndConds must be
     * called before calling this method
     */
    void computeAndSaveTo(std::vector<Degree6Polynomial> &results);

private:
    CoefficientSet s1c_;
    Polynomial s1p0_;
    EquationSet s1e_;
    double y0_;
    double y1_;
    double h0_;
    double d0_;
    double d1_;
    double s0_;
    double s1_;
    double j0_;
};

/**
 * \brief Class for computing Spline of 5 polynomials with C3 continuity
 *
 * The spline is defined by values in 6 points, initial first, second and
 * third derivative and final first and second derivative.
 *
 * Construction of this class requires an allocation. It is recomended to
 * construct it and then use it multiple times
 */
class SplineC3S5 {
public:
    SplineC3S5();

    void setXs(double a_x0, double a_x1, double a_x2, double a_x3, double a_x4, double a_x5);
    void setYs(double a_y0, double a_y1, double a_y2, double a_y3, double a_y4, double a_y5);
    /**
     * \brief sets first, second and third derivative at point x0
     */
    void setStartConds(double a_d0, double a_s0, double a_j0);
    /**
     * \brief sets first and second derivative at point x5
     *
     * usually set to 0 (stopping the motion)
     */
    void setEndConds(double a_d5, double a_s5, double a_j5);
    /**
     * \brief performs spline computation for inputs set by set_* methods
     * \param[out] results pushes computed polynomials to this vector
     *
     * methods setXs, setYs, setStartConds and setEndConds must be
     * called before calling this method
     */
    void computeAndSaveTo(std::vector<Degree7Polynomial> &results);

private:
    CoefficientSet s3c_;
    Polynomial s3p0_;
    Polynomial s3p1_;
    Polynomial s3p2_;
    Polynomial s3p3_;
    Polynomial s3p4_;
    EquationSet s3e_;
    double y0_;
    double y1_;
    double y2_;
    double y3_;
    double y4_;
    double y5_;
    double h0_;
    double h1_;
    double h2_;
    double h3_;
    double h4_;
    double d0_;
    double d5_;
    double s0_;
    double s5_;
    double j0_;
    double j5_;
};

/**
 * \brief Class for computing Spline of 4 polynomials with C3 continuity
 *
 * The spline is defined by values in 5 points, initial first, second and
 * third derivative and final first and second derivative.
 *
 * Construction of this class requires an allocation. It is recomended to
 * construct it and then use it multiple times
 */
class SplineC3S4 {
public:
    SplineC3S4();

    void setXs(double a_x0, double a_x1, double a_x2, double a_x3, double a_x4);
    void setYs(double a_y0, double a_y1, double a_y2, double a_y3, double a_y4);
    /**
     * \brief sets first, second and third derivative at point x0
     */
    void setStartConds(double a_d0, double a_s0, double a_j0);
    /**
     * \brief sets first and second derivative at point x4
     *
     * usually set to 0 (stopping the motion)
     */
    void setEndConds(double a_d4, double a_s4, double a_j4);
    /**
     * \brief performs spline computation for inputs set by set_* methods
     * \param[out] results pushes computed polynomials to this vector
     *
     * methods setXs, setYs, setStartConds and setEndConds must be
     * called before calling this method
     */
    void computeAndSaveTo(std::vector<Degree7Polynomial> &results);

private:
    CoefficientSet s3c_;
    Polynomial s3p0_;
    Polynomial s3p1_;
    Polynomial s3p2_;
    Polynomial s3p3_;
    EquationSet s3e_;
    double y0_;
    double y1_;
    double y2_;
    double y3_;
    double y4_;
    double h0_;
    double h1_;
    double h2_;
    double h3_;
    double d0_;
    double d4_;
    double s0_;
    double s4_;
    double j0_;
    double j4_;
};

/**
 * \brief Class for computing Spline of 3 polynomials with C3 continuity
 *
 * The spline is defined by values in 4 points, initial first, second and
 * third derivative and final first and second derivative.
 *
 * Construction of this class requires an allocation. It is recomended to
 * construct it and then use it multiple times
 */
class SplineC3S3 {
public:
    SplineC3S3();

    void setXs(double a_x0, double a_x1, double a_x2, double a_x3);
    void setYs(double a_y0, double a_y1, double a_y2, double a_y3);
    /**
     * \brief sets first, second and third derivative at point x0
     */
    void setStartConds(double a_d0, double a_s0, double a_j0);
    /**
     * \brief sets first and second derivative at point x3
     *
     * usually set to 0 (stopping the motion)
     */
    void setEndConds(double a_d3, double a_s3, double a_j3);
    /**
     * \brief performs spline computation for inputs set by set_* methods
     * \param[out] results pushes computed polynomials to this vector
     *
     * methods setXs, setYs, setStartConds and setEndConds must be
     * called before calling this method
     */
    void computeAndSaveTo(std::vector<Degree7Polynomial> &results);

private:
    CoefficientSet s3c_;
    Polynomial s3p0_;
    Polynomial s3p1_;
    Polynomial s3p2_;
    EquationSet s3e_;
    double y0_;
    double y1_;
    double y2_;
    double y3_;
    double h0_;
    double h1_;
    double h2_;
    double d0_;
    double d3_;
    double s0_;
    double s3_;
    double j0_;
    double j3_;
};

/**
 * \brief Class for computing Spline of 2 polynomials with C3 continuity
 *
 * The spline is defined by values in 3 points, initial first, second and
 * third derivative and final first and second derivative.
 *
 * Construction of this class requires an allocation. It is recomended to
 * construct it and then use it multiple times
 */
class SplineC3S2 {
public:
    SplineC3S2();

    void setXs(double a_x0, double a_x1, double a_x2);
    void setYs(double a_y0, double a_y1, double a_y2);
    /**
     * \brief sets first and second derivative at point x0
     */
    void setStartConds(double a_d0, double a_s0, double a_j0);
    /**
     * \brief sets first and second derivative at point x2
     *
     * usually set to 0 (stopping the motion)
     */
    void setEndConds(double a_d2, double a_s2, double a_j2);
    /**
     * \brief performs spline computation for inputs set by set_* methods
     * \param[out] results pushes computed polynomials to this vector
     *
     * methods setXs, setYs, setStartConds and setEndConds must be
     * called before calling this method
     */
    void computeAndSaveTo(std::vector<Degree7Polynomial> &results);

private:
    CoefficientSet s2c_;
    Polynomial s2p0_;
    Polynomial s2p1_;
    EquationSet s2e_;
    double y0_;
    double y1_;
    double y2_;
    double h0_;
    double h1_;
    double d0_;
    double d2_;
    double s0_;
    double s2_;
    double j0_;
    double j2_;
};

/**
 * \brief Class for computing Spline of 1 polynomial with C3 continuity
 *
 * The spline is defined by values in 2 points, initial first, second and
 * third derivative and final first and second derivative.
 *
 * Construction of this class requires an allocation. It is recomended to
 * construct it and then use it multiple times
 */
class SplineC3S1 {
public:
    SplineC3S1();

    void setXs(double a_x0, double a_x1);
    void setYs(double a_y0, double a_y1);
    /**
     * \brief sets first and second derivative at point x0
     */
    void setStartConds(double a_d0, double a_s0, double a_j0);
    /**
     * \brief sets first and second derivative at point x2
     *
     * usually set to 0 (stopping the motion)
     */
    void setEndConds(double a_d1, double a_s1, double a_j1);
    /**
     * \brief performs spline computation for inputs set by set_* methods
     * \param[out] results pushes computed polynomials to this vector
     *
     * methods setXs, setYs, setStartConds and setEndConds must be
     * called before calling this method
     */
    void computeAndSaveTo(std::vector<Degree7Polynomial> &results);

private:
    CoefficientSet s1c_;
    Polynomial s1p0_;
    EquationSet s1e_;
    double y0_;
    double y1_;
    double h0_;
    double d0_;
    double d1_;
    double s0_;
    double s1_;
    double j0_;
    double j1_;
};

/**
 * \brief Class for computing Spline of 4 polynomials with C3 continuity
 *
 * The spline is defined by values in 5 points, initial first, second and
 * third derivative and final first and second derivative.
 *
 * Construction of this class requires an allocation. It is recomended to
 * construct it and then use it multiple times
 */
class SplineC3xS4 {
public:
    SplineC3xS4();

    void setXs(double a_x0, double a_x1, double a_x2, double a_x3, double a_x4);
    void setYs(double a_y0, double a_y1, double a_y2, double a_y3, double a_y4);
    /**
     * \brief sets first, second and third derivative at point x0
     */
    void setStartConds(double a_d0, double a_s0, double a_j0);
    /**
     * \brief sets first and second derivative at point x3
     *
     * usually set to 0 (stopping the motion)
     */
    void setEndConds(double a_d3, double a_s3);
    /**
     * \brief performs spline computation for inputs set by set_* methods
     * \param[out] results pushes computed polynomials to this vector
     *
     * methods setXs, setYs, setStartConds and setEndConds must be
     * called before calling this method
     */
    void computeAndSaveTo(std::vector<Degree6Polynomial> &results);

private:
    CoefficientSet s3c_;
    Polynomial s3p0_;
    Polynomial s3p1_;
    Polynomial s3p2_;
    Polynomial s3p3_;
    EquationSet s3e_;
    double y0_;
    double y1_;
    double y2_;
    double y3_;
    double y4_;
    double h0_;
    double h1_;
    double h2_;
    double h3_;
    double d0_;
    double d3_;
    double s0_;
    double s3_;
    double j0_;
};

/**
 * \brief Class for computing Spline of 3 polynomials with C3 continuity
 *
 * The spline is defined by values in 4 points, initial first, second and
 * third derivative and final first and second derivative.
 *
 * Construction of this class requires an allocation. It is recomended to
 * construct it and then use it multiple times
 */
class SplineC3xS3 {
public:
    SplineC3xS3();

    void setXs(double a_x0, double a_x1, double a_x2, double a_x3);
    void setYs(double a_y0, double a_y1, double a_y2, double a_y3);
    /**
     * \brief sets first, second and third derivative at point x0
     */
    void setStartConds(double a_d0, double a_s0, double a_j0);
    /**
     * \brief sets first and second derivative at point x3
     *
     * usually set to 0 (stopping the motion)
     */
    void setEndConds(double a_d3, double a_s3);
    /**
     * \brief performs spline computation for inputs set by set_* methods
     * \param[out] results pushes computed polynomials to this vector
     *
     * methods setXs, setYs, setStartConds and setEndConds must be
     * called before calling this method
     */
    void computeAndSaveTo(std::vector<Degree6Polynomial> &results);

private:
    CoefficientSet s3c_;
    Polynomial s3p0_;
    Polynomial s3p1_;
    Polynomial s3p2_;
    EquationSet s3e_;
    double y0_;
    double y1_;
    double y2_;
    double y3_;
    double h0_;
    double h1_;
    double h2_;
    double d0_;
    double d3_;
    double s0_;
    double s3_;
    double j0_;
};

/**
 * \brief Class for computing Spline of 2 polynomials with C3 continuity
 *
 * The spline is defined by values in 3 points, initial first, second and
 * third derivative and final first and second derivative.
 *
 * Construction of this class requires an allocation. It is recomended to
 * construct it and then use it multiple times
 */
class SplineC3xS2 {
public:
    SplineC3xS2();

    void setXs(double a_x0, double a_x1, double a_x2);
    void setYs(double a_y0, double a_y1, double a_y2);
    /**
     * \brief sets first and second derivative at point x0
     */
    void setStartConds(double a_d0, double a_s0, double a_j0);
    /**
     * \brief sets first and second derivative at point x2
     *
     * usually set to 0 (stopping the motion)
     */
    void setEndConds(double a_d2, double a_s2);
    /**
     * \brief performs spline computation for inputs set by set_* methods
     * \param[out] results pushes computed polynomials to this vector
     *
     * methods setXs, setYs, setStartConds and setEndConds must be
     * called before calling this method
     */
    void computeAndSaveTo(std::vector<Degree6Polynomial> &results);

private:
    CoefficientSet s2c_;
    Polynomial s2p0_;
    Polynomial s2p1_;
    EquationSet s2e_;
    double y0_;
    double y1_;
    double y2_;
    double h0_;
    double h1_;
    double d0_;
    double d2_;
    double s0_;
    double s2_;
    double j0_;
};

/**
 * \brief Class for computing Spline of 1 polynomial with C3 continuity
 *
 * The spline is defined by values in 2 points, initial first, second and
 * third derivative and final first and second derivative.
 *
 * Construction of this class requires an allocation. It is recomended to
 * construct it and then use it multiple times
 */
class SplineC3xS1 {
public:
    SplineC3xS1();

    void setXs(double a_x0, double a_x1);
    void setYs(double a_y0, double a_y1);
    /**
     * \brief sets first and second derivative at point x0
     */
    void setStartConds(double a_d0, double a_s0, double a_j0);
    /**
     * \brief sets first and second derivative at point x2
     *
     * usually set to 0 (stopping the motion)
     */
    void setEndConds(double a_d1, double a_s1);
    /**
     * \brief performs spline computation for inputs set by set_* methods
     * \param[out] results pushes computed polynomials to this vector
     *
     * methods setXs, setYs, setStartConds and setEndConds must be
     * called before calling this method
     */
    void computeAndSaveTo(std::vector<Degree6Polynomial> &results);

private:
    CoefficientSet s1c_;
    Polynomial s1p0_;
    EquationSet s1e_;
    double y0_;
    double y1_;
    double h0_;
    double d0_;
    double d1_;
    double s0_;
    double s1_;
    double j0_;
};

/**
 * \brief Class for computing Spline of 2 polynomials with C1 continuity with derivative also set in the intermediate
 * point
 *
 * The spline is defined by values in 3 points, and first derivative in all 3 points
 *
 * Construction of this class requires an allocation. It is recomended to
 * construct it and then use it multiple times
 */
class SplineC1S2I {
public:
    SplineC1S2I();

    void setXs(double a_x0, double a_x1, double a_x2);
    void setYs(double a_y0, double a_y1, double a_y2);
    void setDerivatives(double a_d0, double a_d1, double a_d2);
    /**
     * \brief performs spline computation for inputs set by set_* methods
     * \param[out] result0 first computed polynomial
     * \param[out] result1 first computed polynomial
     *
     * methods setXs, setYs, setStartConds and setEndConds must be
     * called before calling this method
     */
    void computeAndSaveTo(Degree3Polynomial &result0, Degree3Polynomial &result1);

private:
    CoefficientSet c_;
    Polynomial p0_;
    Polynomial p1_;
    EquationSet e_;
    double y0_;
    double y1_;
    double y2_;
    double h0_;
    double h1_;
    double d0_;
    double d1_;
    double d2_;
};

} // namespace kswx_weaving_generator

#endif // WEAVING_GENERATOR_SPLINE_COMPUTATION_H
