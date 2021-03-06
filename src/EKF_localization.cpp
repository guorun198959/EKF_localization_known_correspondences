#include "EKF_localization.h"

#include <iostream>

#include "config.h"
#include "robot.h"

using namespace std;
using namespace Eigen;

EKF_localization::EKF_localization()
{
    init_ekf();
}

void EKF_localization::init_ekf()
{
    m_cov.setZero();
}

void EKF_localization::set_state(double x, double y, double yaw)
{
    m_mu(0) = x;
    m_mu(1) = y;
    m_mu(2) = yaw;
}

void EKF_localization::update(double v, double w, const std::vector<Landmark> &landmarks, double dt)
{
    // Implementation based on
    // http://probabilistic-robotics.informatik.uni-freiburg.de/corrections1/pg204-206.pdf
    // I've kept variables name to be the same where possible.

    // I added special handling for w ~ 0. The original implementation does not handle this.
    const double EPS = 1e-4;

    Matrix<double, 3, 3> G; // Jacobian (motion update / state variable)
    Matrix<double, 3, 2> V; // Jacobian (motion noise / state variable)
    Matrix<double, 2, 2> M; // motion noise
    Matrix<double, 3, 1> MU;
    Matrix<double, 3, 3> COV;

    // correction step
    Matrix<double, 2, 1> ZHAT; // expected sensor measurement
    Matrix<double, 2, 1> Z; // sensor measurement
    Matrix<double, 2, 3> H; // Jacobian
    Matrix<double, 2, 2> S;
    Matrix<double, 2, 2> Q; // measurement noise
    Matrix<double, 3, 2> K; // Kalman gain
    Matrix<double, 3, 3> I; // Identity

    G.setIdentity();
    V.setZero();
    M.setZero();
    Q.setZero();
    H.setZero();
    I.setIdentity();

    m_dt = dt;

    double theta = yaw(); // previous yaw

    // noise
    M(0, 0) = pow(ALPHA1*fabs(v) + ALPHA2*fabs(w), 2);
    M(1, 1) = pow(ALPHA3*fabs(v) + ALPHA4*fabs(w), 2);

    // check if angular velocity is close to zero
    if (fabs(w) > EPS) {
        // Jacobian
        G(0, 2) = -(v/w)*cos(theta) + (v/w)*cos(theta + w*dt);
        G(1, 2) = -(v/w)*sin(theta) + (v/w)*sin(theta + w*dt);

        V(0, 0) = (-sin(theta) + sin(theta + w*dt))/w;
        V(1, 0) = ( cos(theta) - cos(theta + w*dt))/w;
        V(0, 1) =  v*(sin(theta) - sin(theta + w*dt))/(w*w) + v*cos(theta + w*dt)*dt/w;
        V(1, 1) = -v*(cos(theta) - cos(theta + w*dt))/(w*w) + v*sin(theta + w*dt)*dt/w;
        V(2, 0) = 0;
        V(2, 1) = dt;

        // Prediction
        MU(0) = m_mu(0) - (v/w)*sin(theta) + (v/w)*sin(theta + w*dt);
        MU(1) = m_mu(1) + (v/w)*cos(theta) - (v/w)*cos(theta + w*dt);
        MU(2) = m_mu(2) + w*dt;

        COV = G*m_cov*G.transpose() + V*M*V.transpose();
    } else {
        // Handle case when w ~ 0
        // Use L'Hopital rule with lim w -> 0
        G(0, 2) = -v*sin(theta)*dt;
        G(1, 2) =  v*cos(theta)*dt;

        V(0, 0) = cos(theta)*dt;
        V(1, 0) = sin(theta)*dt;
        V(0, 1) = -v*sin(theta)*dt*dt*0.5;
        V(1, 1) =  v*cos(theta)*dt*dt*0.5;
        V(2, 0) = 0;
        V(2, 1) = dt;

        MU(0) = m_mu(0) + v*cos(theta)*dt;
        MU(1) = m_mu(1) + v*sin(theta)*dt;
        MU(2) = m_mu(2);

        COV = G*m_cov*G.transpose() + V*M*V.transpose();
    }

    for (const auto &l : landmarks) {
        Z(0) = l.range;
        Z(1) = l.bearing;

        if (fabs(l.range) > EPS) {
            double range, bearing;
            Robot::landmark_range_bearing(l, MU(0), MU(1), MU(2), range, bearing);

            ZHAT(0) = range;
            ZHAT(1) = bearing;

            H(0, 0) = -(l.x - MU(0))/range;
            H(0, 1) = -(l.y - MU(1))/range;
            H(0, 2) = 0;
            H(1, 0) =  (l.y - MU(1))/(range*range);
            H(1, 1) = -(l.x - MU(0))/(range*range);
            H(1, 2) =  -1;

            Q(0, 0) = pow(l.range*DETECTION_RANGE_ALPHA, 2);
            Q(1, 1) = pow(DETECTION_ANGLE_SIGMA, 2);

            S = H*COV*H.transpose() + Q;

            K = COV*H.transpose()*S.inverse();
            MU = MU + K*(Z - ZHAT);
            COV = (I - K*H)*COV;

            MU(2) = constrain_angle(MU(2));
        }
    }

    m_mu = MU;
    m_cov = COV;
}

double EKF_localization::constrain_angle(double radian)
{
    if (radian < -M_PI) {
        radian += 2*M_PI;
    } else if (radian > M_PI) {
        radian -= 2*M_PI;
    }

    return radian;
}

void EKF_localization::ellipse(Eigen::MatrixXd X, double &major, double &minor, double &theta)
{
    SelfAdjointEigenSolver<Matrix2d> a(X);

    double e0 = sqrt(a.eigenvalues()(0));
    double e1 = sqrt(a.eigenvalues()(1));

    if (e0 > e1) {
        theta = atan2(a.eigenvectors()(1, 0), a.eigenvectors()(0, 0));
        major = e0;
        minor = e1;
    } else {
        theta = atan2(a.eigenvectors()(1, 1), a.eigenvectors()(0, 1));
        major = e1;
        minor = e0;
    }
}

void EKF_localization::pose_ellipse(double &major, double &minor, double &theta)
{
    ellipse(m_cov.block(0, 0, 2, 2), major, minor, theta);
}
