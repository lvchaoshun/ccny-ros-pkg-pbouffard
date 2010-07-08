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

#include <ground_station/ground_station.h>

AppData *data;
double value_vel = 0, value_batt = 0, val =100,val2=12;

void updateAltimeterCallback (const geometry_msgs::PoseConstPtr & msg)
{
  int angle;

  // **** get GTK thread lock
  gdk_threads_enter ();

  // **** update altimeter
  if (IS_GTK_ALTIMETER (data->alt))
  {
    gtk_altimeter_set_alti (GTK_ALTIMETER (data->alt), msg->position.z);
  }
  
    // **** update variometer
  if (IS_GTK_VARIOMETER (data->vario))
		gtk_variometer_set_value (GTK_VARIOMETER (data->vario), msg->position.z);		

  // **** update compass
  if (IS_GTK_COMPASS (data->comp))
  {
    angle = (int) msg->position.z / 10 % 360;
    gtk_compass_set_angle (GTK_COMPASS (data->comp), angle);
  }

  // **** update gauge
  if(IS_GTK_GAUGE(data->vel))         
  {
  if(value_vel/10 <74)        value_vel=value_vel+11;
  else if(value_vel/10>50) value_vel=value_vel-11;
  gtk_gauge_set_value(GTK_GAUGE(data->vel), value_vel/10);
  }
  
  // **** update gauge
  if(IS_GTK_GAUGE(data->battery))             
  {
  if(value_batt/10 <9)        value_batt++;
  else if(value_vel/10>12) value_batt--;
  gtk_gauge_set_value(GTK_GAUGE(data->battery), value_batt/10);
  }
    
  // **** update bar gauge
  if(IS_GTK_BAR_GAUGE(data->bg))             
  {
  val--;
  if(val==0) val = 100;
  val2=val2-0.01;
  if(val2<=0) val2 = 12;
  gtk_bar_gauge_set_value(GTK_BAR_GAUGE(data->bg),1, val2);
  gtk_bar_gauge_set_value(GTK_BAR_GAUGE(data->bg),2, val);
  }

  // **** release GTK thread lock 
  gdk_threads_leave ();
}

void *startROS (void *user)
{
  if (user != NULL)
  {
    struct arg *p_arg = (arg *) user;

    ros::init (p_arg->argc, p_arg->argv, "ground_station");
    ros::NodeHandle n;

    std::string local_path;
    std::string package_path = ros::package::getPath (ROS_PACKAGE_NAME);
    ros::NodeHandle n_param ("~");
    XmlRpc::XmlRpcValue xml_marker_center;

    ROS_INFO ("Starting CityFlyer Ground Station");

    // **** get parameters
    if (!n_param.getParam ("window_inv_color", data->inv_color))
      data->inv_color = false;
    ROS_DEBUG ("\tWindow color inversed: %d", data->inv_color);    
    
    if (!n_param.getParam ("altimeter_unit_is_feet", data->altimeter_unit_is_feet))
      data->altimeter_unit_is_feet = true;
    ROS_DEBUG ("\tAltimeter unit is FEET: %d", data->altimeter_unit_is_feet);

    if (!n_param.getParam ("altimeter_step_value", data->altimeter_step_value))
      data->altimeter_step_value = 100;
    ROS_DEBUG ("\tAltimeter step value: %d", data->altimeter_step_value);

    // **** allow widget creation
    data->ros_param_read = true;

    // **** wait to widget creation
    while (!data->widget_created)
    {
      ROS_DEBUG ("Waiting widgets creation");
    }

    // **** topics subscribing
    ros::Subscriber sub = n.subscribe ("fake_alti", 1, updateAltimeterCallback);

    ROS_INFO ("Spinning");
    ros::spin ();
  }

  // **** stop the gtk main loop
  if (GTK_IS_WINDOW (data->window))
  {
    gtk_main_quit ();
  }
  pthread_exit (NULL);
}

gboolean window_update(gpointer dat)
{	
  // **** update altimeter
  if (IS_GTK_ALTIMETER (data->alt))
		gtk_altimeter_redraw (GTK_ALTIMETER (data->alt));
		
  // **** update variometer
  if (IS_GTK_VARIOMETER (data->vario))
		gtk_variometer_redraw (GTK_VARIOMETER (data->vario));		

  // **** update compass
  if (IS_GTK_COMPASS (data->comp))
		gtk_compass_redraw (GTK_COMPASS (data->comp));

  // **** update gauge
  if(IS_GTK_GAUGE(data->vel))       
		gtk_gauge_redraw(GTK_GAUGE(data->vel));
  
  // **** update gauge
  if(IS_GTK_GAUGE(data->battery))             
		gtk_gauge_redraw(GTK_GAUGE(data->battery));
    
  // **** update bar gauge
  if(IS_GTK_BAR_GAUGE(data->bg))             
		gtk_bar_gauge_redraw(GTK_BAR_GAUGE(data->bg));

  return true;
}

int main (int argc, char **argv)
{
  GtkWidget *vbox;
  GtkWidget *hbox_up;
  GtkWidget *hbox_down;
  GdkColor bg_color;

  struct arg param;
  param.argc = argc;
  param.argv = argv;

  pthread_t rosThread;

  // **** init threads 
  g_thread_init (NULL);
  gdk_threads_init ();
  gdk_threads_enter ();

  // **** init gtk 
  gtk_init (&argc, &argv);

  // **** allocate data structure
  data = g_slice_new (AppData);

  // **** create window
  data->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (data->window), "CityFlyer Ground Station");
  gtk_window_set_position (GTK_WINDOW (data->window), GTK_WIN_POS_CENTER);
  gtk_window_set_default_size (GTK_WINDOW (data->window), 1200, 700);

  g_signal_connect (G_OBJECT (data->window), "destroy", G_CALLBACK (gtk_main_quit), NULL);

  vbox = gtk_vbox_new (TRUE, 1);
  gtk_container_add (GTK_CONTAINER (data->window), vbox);

  hbox_up = gtk_hbox_new (TRUE, 1);
  gtk_container_add (GTK_CONTAINER (vbox), hbox_up);

  hbox_down = gtk_hbox_new (TRUE, 1);
  gtk_container_add (GTK_CONTAINER (vbox), hbox_down);

  // **** create ROS thread
  pthread_create (&rosThread, NULL, startROS, &param);

  // **** wait ros finish read params
  while (!data->ros_param_read)
  {
    ROS_DEBUG ("Waiting ROS params");
  }

  // **** create altimeter widgets
  data->alt = gtk_altimeter_new ();
  g_object_set (GTK_ALTIMETER (data->alt), 
					 "inverse-color", data->inv_color,
                "unit-is-feet", data->altimeter_unit_is_feet,
                "unit-step-value", data->altimeter_step_value, 
                "radial-color", true, NULL);

  // **** create compass widgets
  data->comp = gtk_compass_new ();
  g_object_set (GTK_COMPASS (data->comp), 
					 "inverse-color", data->inv_color, 
					 "radial-color", true, NULL);

  // **** create gauge widgets
  data->vel = gtk_gauge_new ();
  g_object_set (GTK_GAUGE (data->vel), "name",
                "<big>Gauge X</big>\n" "<span foreground=\"orange\"><i>(value)</i></span>", NULL);
  
  g_object_set (GTK_GAUGE (data->vel), 
					 "inverse-color", data->inv_color,
                "radial-color", true,
                "start-value", 0,
                "end-value", 1000, 
                "initial-step", 200, 
                "sub-step", (gdouble) 50.0, 
                "drawing-step", 200,                                                                                                                                                                                                                                                                                                          
                NULL);

  // **** create gauge widgets
  data->battery = gtk_gauge_new ();
  g_object_set (GTK_GAUGE (data->battery), "name",
                "<big>Battery voltage</big>\n" "<span foreground=\"orange\"><i>(V)</i></span>", NULL);
  g_object_set (GTK_GAUGE (data->battery),
					 "inverse-color", data->inv_color,
                "radial-color", true,
                "start-value", 0, 
                "end-value", 12, 
                "initial-step", 2, 
                "sub-step", (gdouble) 0.2, 
                "drawing-step", 2,
                "color-strip-order", "RYG",
                "green-strip-start", 10,
                "yellow-strip-start", 8,
                "red-strip-start", 0,                                                                                                                                                                                                                    
                NULL);
                
  // **** create gauge widgets
  data->bg = gtk_bar_gauge_new ();
  g_object_set (GTK_BAR_GAUGE (data->bg), "name-bar-1",
                "<big>Bat</big>\n" "<span foreground=\"orange\"><i>(V)</i></span>", NULL);
  g_object_set (GTK_BAR_GAUGE (data->bg), "name-bar-2",
                "<big>Aux</big>\n" "<span foreground=\"orange\"><i>(?)</i></span>", NULL);     
  g_object_set (GTK_BAR_GAUGE (data->bg), "name-bar-3",
                "<big>Aux2</big>\n" "<span foreground=\"orange\"><i>(?)</i></span>", NULL);  
  g_object_set (GTK_BAR_GAUGE (data->bg),
					 "bar-number", 3,
					 "inverse-color", data->inv_color,
                "radial-color", true,
                "start-value-bar-1", 0, 
                "end-value-bar-1", 12, 
                "green-strip-start-1", 10,
                "yellow-strip-start-1", 8,
                "start-value-bar-2", 0, 
                "end-value-bar-2", 100,
                "start-value-bar-3", 0, 
                "end-value-bar-3", 100,      
                "start-value-bar-4", 0, 
                "end-value-bar-4", 100,                                                                                                                                                                                                                                         
                NULL);
                
  // **** create artificial horizon widgets
  data->arh = gtk_artificial_horizon_new ();
  g_object_set (GTK_ARTIFICIAL_HORIZON (data->arh), 
					 "inverse-color", data->inv_color,
                "radial-color", true, NULL);
                
  // **** create variometer widgets
  data->vario = gtk_variometer_new ();
  g_object_set (GTK_VARIOMETER (data->vario), 
					 "inverse-color", data->inv_color,
                "unit-is-feet", data->altimeter_unit_is_feet,
                "unit-step-value", 1000, 
                "radial-color", true, NULL);
         
  // **** create variometer widgets
  data->tc = gtk_turn_coordinator_new ();
  g_object_set (GTK_TURN_COORDINATOR (data->tc), 
					 "inverse-color", data->inv_color,
                "unit-is-feet", data->altimeter_unit_is_feet,
                "unit-step-value", 1000, 
                "radial-color", true, NULL);                

  gtk_box_pack_start (GTK_BOX (hbox_up), GTK_WIDGET (data->vel), TRUE, TRUE, 0);    
  gtk_box_pack_start (GTK_BOX (hbox_up), GTK_WIDGET (data->alt), TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox_up), GTK_WIDGET (data->arh), TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox_up), GTK_WIDGET (data->bg), TRUE, TRUE, 0);    
    
  gtk_box_pack_start (GTK_BOX (hbox_down), GTK_WIDGET (data->tc), TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox_down), GTK_WIDGET (data->battery), TRUE, TRUE, 0);  
  gtk_box_pack_start (GTK_BOX (hbox_down), GTK_WIDGET (data->comp), TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox_down), GTK_WIDGET (data->vario), TRUE, TRUE, 0);

  bg_color.red=0.4*65535;
  bg_color.green=0.4*65535;
  bg_color.blue=0.4*65535;
  gtk_widget_modify_bg (data->window, GTK_STATE_NORMAL, &bg_color);
  gtk_widget_modify_bg (data->alt, GTK_STATE_NORMAL, &bg_color);
  gtk_widget_modify_bg (data->comp, GTK_STATE_NORMAL, &bg_color);
  gtk_widget_modify_bg (data->battery, GTK_STATE_NORMAL, &bg_color);
  gtk_widget_modify_bg (data->vel, GTK_STATE_NORMAL, &bg_color);  
  gtk_widget_modify_bg (data->arh, GTK_STATE_NORMAL, &bg_color);
  gtk_widget_modify_bg (data->bg, GTK_STATE_NORMAL, &bg_color);
  gtk_widget_modify_bg (data->vario, GTK_STATE_NORMAL, &bg_color);
  gtk_widget_modify_bg (data->tc, GTK_STATE_NORMAL, &bg_color);  
  
  gtk_widget_show_all (data->window);

  // **** allow ROS spinning
  data->widget_created = true;
  
  // **** udpate all widget
  //g_timeout_add (200, window_update, NULL);

  gtk_main ();
  gdk_threads_leave ();
  return 0;
}
