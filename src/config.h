#pragma once

#include "landmark.h"

// All metric units are in pixels.

// world settings
static const int WORLD_WIDTH = 600;
static const int WORLD_HEIGHT= 600;

static const int BOUNDARY_X1 = 50;
static const int BOUNDARY_X2 = 550;
static const int BOUNDARY_Y1 = 50;
static const int BOUNDARY_Y2 = 550;

static const Landmark LANDMARKS[] = {
    // (x, y, red, green, blue)
    {100, 100, 1, 0, 0},
    {WORLD_WIDTH - 100, 100, 1, 0,  0},
    {WORLD_WIDTH - 100, WORLD_HEIGHT - 100, 1, 0,  0},
    {100, WORLD_HEIGHT - 100, 1, 0,  0},
    {WORLD_WIDTH/2, WORLD_HEIGHT/2, 1, 0,  0},
};

// landmark detection noise parameters
static const double LANDMARK_RANGE_SIGMA = 20.0;
static const double LANDMARK_ANGLE_SIGMA = 2*M_PI/180;

// motion noise parameters
static const double ALPHA1 = 0.1;
static const double ALPHA2 = 0;
static const double ALPHA3 = 0.0001;
static const double ALPHA4 = 0.1;

// robot motion properties
static const double ROBOT_VEL = 100; // pixels/sec
static const double ROBOT_YAW_VEL = 60*M_PI/180; // rad/sec

// properties of the robot's sensor
static const double FOV = 45*M_PI/180;
static const double DETECTION_RANGE = 200;
static const double DETECTION_RANGE_ALPHA = 0.1;
static const double DETECTION_ANGLE_SIGMA = 2*M_PI/180;

// draw 95% confidence ellipse
static const double ELLIPSE_CHI = 2.4477;
