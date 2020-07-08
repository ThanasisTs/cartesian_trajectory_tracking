#include "reactive_control/reactive_control.h"

geometry_msgs::TwistStampedPtr robot_velocity = boost::make_shared<geometry_msgs::TwistStamped>();
geometry_msgs::TwistPtr desired_robot_velocity = boost::make_shared<geometry_msgs::Twist>();
std_msgs::Float64 time_duration_msg;
std::vector<float> acc;
float vel_duration;
float K, dis_x, dis_y, dis_z;
float time_duration, time_init;
ros::Time keypoint_time;
ros::Publisher time_pub;

float euclidean_distance (std::shared_ptr<std::vector<float>> v1, std::shared_ptr<std::vector<float>> v2){
	float temp = 0;
	for (short int i=0; i<v1->size(); i++){
		temp += pow((v1->at(i) - v2->at(i)), 2);
	}
	return sqrt(temp);
}

void human_motion_callback(const geometry_msgs::PointStampedConstPtr human_msg){
	received_point = true;
	if (count == 0){
		init_x = human_msg->point.x + xOffset;
		init_y = human_msg->point.y + yOffset;
		init_z = human_msg->point.z + zOffset;
		time_init = ros::Time::now().toSec();
	}
	else{
		if (count == 1){
			start_time = ros::Time::now().toSec();
			dis_x = human_msg->point.x + xOffset - init_x;
			dis_y = human_msg->point.y + yOffset - init_y;
			dis_z = human_msg->point.z + zOffset - init_z;
			std::cout << dis_z << std::endl;
		}
		timenow = ros::Time::now();
		desired_robot_position->header.stamp = timenow;
		keypoint_time = ros::Time::now();
		robot_state->header.stamp = timenow;
		state_pub_low_f.publish(*robot_state);
		
		vis_robot_pub.publish(*marker_robot);
	  	// ROS_INFO("Num of low freq robot state: %d", count);
		dis.data = sqrt(pow(desired_robot_position->point.x - robot_state->pose.position.x, 2) 
			+ pow(desired_robot_position->point.y - robot_state->pose.position.y, 2));
		dis_pub.publish(dis);
		marker_robot->points.push_back(robot_state->pose.position);
		init_point = true;
		if (count == 1){
			time_duration = keypoint_time.toSec() - time_init;
			if (time_duration != 0){
				desired_robot_velocity->linear.x = (human_msg->point.x + xOffset - dis_x - init_x)/time_duration;
				desired_robot_velocity->linear.y = (human_msg->point.y + yOffset - dis_x - init_y)/time_duration;
				desired_robot_velocity->linear.z = (human_msg->point.z + zOffset - dis_x - init_z)/time_duration;
			}
		}
		else{

			time_duration = keypoint_time.toSec() - desired_robot_position->header.stamp.toSec();
			time_duration_msg.data = time_duration;
			time_pub.publish(time_duration_msg);
			// std::cout << time_duration << std::endl;
			desired_robot_velocity->linear.x = (human_msg->point.x + xOffset - dis_x - desired_robot_position->point.x)/time_duration;
			desired_robot_velocity->linear.y = (human_msg->point.y + yOffset - dis_y - desired_robot_position->point.y)/time_duration;
			desired_robot_velocity->linear.z = (human_msg->point.z + zOffset - dis_z - desired_robot_position->point.z)/time_duration;			
			std::cout << time_duration << std::endl;
			if (std::isnan(desired_robot_velocity->linear.x)){
				std::cout << "Time duration limit exceeded" << std::endl;
				std::cout << time_duration << std::endl;
			}
			// std::cout << *desired_robot_velocity << std::endl;
		}
	}

	count += 1;
	// std::cout << count << std::endl;
	desired_robot_position->point.x = human_msg->point.x + xOffset - dis_x;
	desired_robot_position->point.y = human_msg->point.y + yOffset - dis_y;
	desired_robot_position->point.z = human_msg->point.z + zOffset - dis_z;
	desired_robot_position->header.stamp = ros::Time::now();

	control_points_pub.publish(*desired_robot_position);
	marker_human->header.stamp = ros::Time::now();

    marker_human->points.push_back(desired_robot_position->point);
  	vis_human_pub.publish(*marker_human);
	if (var){
		v1->push_back(robot_state->pose.position.x);
		v1->push_back(robot_state->pose.position.y);
		v1->push_back(robot_state->pose.position.z);
		v2->push_back(desired_robot_position->point.x);
		v2->push_back(desired_robot_position->point.y);
		v2->push_back(desired_robot_position->point.z);
		D = var_gain*euclidean_distance(v1, v2);
		v1->clear();
		v2->clear();
		D_v.push_back(D);
		if (D_v.size() > 1){
			std::cout << std::accumulate(D_v.begin(), D_v.end(), 0.0)/D_v.size() << std::endl;
		}
	}
	if (init_point){
		end_time = ros::Time::now().toSec();
		// ROS_INFO("Time elapsed: %f", end_time-start_time);
	}
}

void state_callback (const trajectory_execution_msgs::PoseTwist::ConstPtr state_msg){
	robot_state->pose.position.x = state_msg->pose.position.x;
	robot_state->pose.position.y = state_msg->pose.position.y;
	robot_state->pose.position.z = state_msg->pose.position.z;
	robot_state->header.stamp = ros::Time::now();

	vel_duration = ros::Time::now().toSec() - robot_velocity->header.stamp.toSec();
	robot_velocity->twist.linear.x = state_msg->twist.linear.x;
	robot_velocity->twist.linear.y = state_msg->twist.linear.y;
	robot_velocity->twist.linear.z = state_msg->twist.linear.z;
	robot_velocity->header.stamp = ros::Time::now();


	if (received_point){
		if (count == 1){
			vel_control->linear.x = (desired_robot_position->point.x - robot_state->pose.position.x);
			vel_control->linear.y = (desired_robot_position->point.y - robot_state->pose.position.y);
			vel_control->linear.z = (desired_robot_position->point.z - robot_state->pose.position.z);
			vel_control->angular.x = 0;
			vel_control->angular.y = 0;
			vel_control->angular.z = 0;
			pub.publish(*vel_control);
		}
		else{
			// std::cout << K << " " << D << std::endl;
			
			vel_command->linear.x = desired_robot_position->point.x - robot_state->pose.position.x;
			vel_command->linear.y = desired_robot_position->point.y - robot_state->pose.position.y;
			vel_command->linear.z = desired_robot_position->point.z - robot_state->pose.position.z;
			vel_pub.publish(*vel_command);

			acc_command->linear.x = desired_robot_velocity->linear.x - robot_velocity->twist.linear.x;
			acc_command->linear.y = desired_robot_velocity->linear.y - robot_velocity->twist.linear.y;
			acc_command->linear.z = desired_robot_velocity->linear.z - robot_velocity->twist.linear.z;
			acc_pub.publish(*acc_command);

			acc[0] = K*(acc_command->linear.x) + D*(vel_command->linear.x);
			acc[1] = K*(acc_command->linear.y) + D*(vel_command->linear.y);
			acc[2] = K*(acc_command->linear.z) + D*(vel_command->linear.z);
			
			// std::cout << "The commanded velocities are\n" << *vel_command << std::endl;
			

			// std::cout << "The commanded accalerations are\n" << *acc_command << std::endl;
			
			vel_control->linear.x = robot_velocity->twist.linear.x + acc[0]*vel_duration;
			vel_control->linear.y = robot_velocity->twist.linear.y + acc[1]*vel_duration;
			vel_control->linear.z = robot_velocity->twist.linear.z + acc[2]*vel_duration;
			// std::cout << *vel_control << std::endl;
			pub.publish(*vel_control);
		}
		if (abs(robot_state->pose.position.x - init_x) < 0.005
		 and abs(robot_state->pose.position.y - init_y) < 0.005 
		 and abs(robot_state->pose.position.z - init_z) < 0.005
		 and not init_point){
		 	ROS_INFO("Reached the first point");
		}
		if (init_point){
			state_pub_high_f.publish(*state_msg);
		}
	}
}


int main(int argc, char** argv){
	ros::init(argc, argv, "reactive_control_node");
	ros::NodeHandle n;
	ros::AsyncSpinner spinner(3);
	spinner.start();
	safe_vel_control->linear.x = 0;
	safe_vel_control->linear.y = 0;
	safe_vel_control->linear.z = 0;
	n.param("reactive_control_node/D", D, 0.0f);
	n.param("reactive_control_node/K", K, 0.0f);
	n.param("reactive_control_node/xOffset", xOffset, 0.0f);
	n.param("reactive_control_node/yOffset", yOffset, 0.0f);
	n.param("reactive_control_node/zOffset", zOffset, 0.0f);
	n.param("reactive_control_node/var", var, false);
	n.param("reactive_control_node/var_gain", var_gain, 10.0f);
	n.param("reactive_control_node/sim", sim, true);
	
	marker_human->header.frame_id = "base_link";
	marker_human->type = visualization_msgs::Marker::LINE_STRIP;
	marker_human->action = visualization_msgs::Marker::ADD;
	marker_human->scale.x = 0.01;
    marker_human->scale.y = 0.01;
    marker_human->scale.z = 0.01;
    marker_human->color.r = 0.0f;
    marker_human->color.g = 1.0f;
    marker_human->color.b = 0.0f;
    marker_human->color.a = 1.0;
  	marker_human->lifetime = ros::Duration(100);
	
	marker_robot->header.frame_id = "base_link";
	marker_robot->header.stamp = ros::Time::now();
	marker_robot->type = visualization_msgs::Marker::LINE_STRIP;
	marker_robot->action = visualization_msgs::Marker::ADD;
	marker_robot->scale.x = 0.01;
    marker_robot->scale.y = 0.01;
    marker_robot->scale.z = 0.01;
    marker_robot->color.r = 0.0f;
    marker_robot->color.g = 0.0f;
    marker_robot->color.b = 1.0f;
    marker_robot->color.a = 1.0;
  	marker_robot->lifetime = ros::Duration(100);
  	

  	desired_robot_velocity->linear.x = 0;
	desired_robot_velocity->linear.y = 0;
  	desired_robot_velocity->linear.z = 0;
  	acc.resize(3);

  	if (sim){
  		ee_state_topic = "/manos_cartesian_velocity_controller_sim/ee_state";
  		ee_vel_command_topic = "/manos_cartesian_velocity_controller_sim/command_cart_vel";	
  	}
  	else{
  		ee_state_topic = "/manos_cartesian_velocity_controller/ee_state";
  		ee_vel_command_topic = "/manos_cartesian_velocity_controller/command_cart_vel";  		
  	}

	pub = n.advertise<geometry_msgs::Twist>(ee_vel_command_topic, 100);
	state_pub_high_f = n.advertise<trajectory_execution_msgs::PoseTwist>("/ee_position_high_f", 100);
	state_pub_low_f = n.advertise<geometry_msgs::PoseStamped>("/ee_position_low_f", 100);
	time_pub = n.advertise<std_msgs::Float64>("/time_topic", 100);
	dis_pub = n.advertise<std_msgs::Float64>("/response_topic", 100);
	vis_human_pub = n.advertise<visualization_msgs::Marker>("/vis_human_topic", 100);
	vis_robot_pub = n.advertise<visualization_msgs::Marker>("/vis_robot_topic", 100);
	control_points_pub = n.advertise<geometry_msgs::PointStamped>("trajectory_points_stamp", 100);	
	vel_pub = n.advertise<geometry_msgs::Twist>("vel_topic", 100);
	acc_pub = n.advertise<geometry_msgs::Accel>("acc_topic", 100);
	
	ros::Subscriber sub = n.subscribe(ee_state_topic, 100, state_callback);
	ros::Subscriber sub2 = n.subscribe("/trajectory_points", 100, human_motion_callback);
	
	ros::waitForShutdown();
}