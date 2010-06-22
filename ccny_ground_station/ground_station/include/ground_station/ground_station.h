/*
 *  Ground Station for CityFlyer CCNY project
 *  Copyright (C) 2010, CCNY Robotics Lab
 *  Gautier Dumonteil <gautier.dumonteil@gmail.com>
 *  http://robotics.ccny.cuny.edu
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CCNY_GROUND_STATION_GROUND_STATION_H
#define CCNY_GROUND_STATION_GROUND_STATION_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include <glib.h>

#include <ros/ros.h>
#include <ros/package.h>
#include <geometry_msgs/Pose.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <ground_station/gui/AppData.h>
#include <ground_station/gui/gtkaltimeter.h>
#include <ground_station/gui/gtkcompass.h>
#include <ground_station/gui/gtkgauge.h>


struct arg
{
  int argc;
  char **argv;
};

void *startGUI (void *);
void *startROS (void *);
void chatterCallback (const geometry_msgs::PoseConstPtr &);

#endif