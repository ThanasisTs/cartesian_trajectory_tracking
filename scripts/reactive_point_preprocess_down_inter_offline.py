#!/usr/bin/env python
import rospy
from geometry_msgs.msg import Point
import sys
import matplotlib.pyplot as plt
from scipy.spatial import distance
import numpy as np
from mpl_toolkits.mplot3d import *

x = []
y = []
z = []
x_inter = []
y_inter = []
z_inter = []
x_all = []
y_all = []
z_all = []
ds_thres = None

def interpolation(p1, p2, dis):
	global x, y, z, x_inter, y_inter, z_inter, ds_thres, x_all, y_all, z_all
	num_inter_points = dis//ds_thres
	print dis, num_inter_points
	for i in np.linspace(0,1,num_inter_points + 1):
		if i==0 or i==1:
			continue
		x_inter.append((1-i)*p1[0] + i*p2[0])
		y_inter.append((1-i)*p1[1] + i*p2[1])
		z_inter.append((1-i)*p1[2] + i*p2[2])
		x.append((1-i)*p1[0] + i*p2[0])
		y.append((1-i)*p1[1] + i*p2[1])
		z.append((1-i)*p1[2] + i*p2[2])
		x_all.append((1-i)*p1[0] + i*p2[0])
		y_all.append((1-i)*p1[1] + i*p2[1])
		z_all.append((1-i)*p1[2] + i*p2[2])


def main():
	global x, y, z, x_inter, y_inter, z_inter, ds_thres, x_all, y_all, z_all
	rospy.init_node("node")
	pub = rospy.Publisher("raw_points", Point, queue_size=10)
	file = open(sys.argv[1], 'r')
	ds_thres = float(sys.argv[2])
	ip_thres = float(sys.argv[3])
	fl = file.readlines()
	title_num = int(sys.argv[-1])
	title_day = sys.argv[-2]
	title = sys.argv[1][title_num:-5] + "_" + sys.argv[-2]
	xRaw, yRaw, zRaw = [], [], []
	count = 0
	count_down = 0
	down_list = []
	for i in xrange(len(fl)):
		if "RWrist" in fl[i]:
			count += 1
			x_tmp = float(fl[i+9][11:])
			y_tmp = float(fl[i+10][11:])
			z_tmp = float(fl[i+11][11:])
			xRaw.append(x_tmp)
			yRaw.append(y_tmp)
			zRaw.append(z_tmp)
			if len(x) >= 1:
				if abs(x[-1] - x_tmp) > 0.1 or abs(y[-1] - y_tmp) > 0.1 or abs(z[-1] - z_tmp) > 0.1:
					continue
				dis = distance.euclidean((x_tmp, y_tmp, z_tmp),(x[-1], y[-1], z[-1]))
			if len(x) == 0 or (len(x) >= 1 and dis >= ds_thres):
				down_list.append(count_down)
				count_down = 0
				if len(x) >= 1 and dis > ip_thres:
					interpolation([x[-1], y[-1], z[-1]], [x_tmp, y_tmp, z_tmp], dis)
				x.append(x_tmp)
				y.append(y_tmp)
				z.append(z_tmp)
			else:
				count_down += 1
	down_list.append(count_down)

	fig = plt.figure()
	ax = plt.axes(projection="3d")
	ax.scatter(xRaw, yRaw, zRaw, s=50)
	ax.scatter(x, y, z, s=20, c="orange")
	dis = []
	for i in xrange(len(x)-1):
		dis_tmp = distance.euclidean((x[i], y[i], z[i]), (x[i+1], y[i+1], z[i+1]))
		dis.append(dis_tmp)
		
	for i in xrange(len(dis)):
		if dis[i] == max(dis):
			ax.text(x[i], y[i], z[i], str(1))
	ax.set_zlim(0.03, 0.06)
	# print max(dis)

	fig, ax = plt.subplots(1,2)
	fig.set_size_inches(20,10)
	ax[0].set_title("Raw, processed and downsampled points for " + str(title) + "\nDownsampling Threshold: " + str(ds_thres) + "m, Interpolation Threshold: " + str(ip_thres) + "m")
	# fig.suptitle("Smart interpolation")
	ax[0].scatter(xRaw, yRaw, s=100, label="Raw points")

	dis = []
	for i in xrange(len(x)-1):
		dis_tmp = distance.euclidean((x[i], y[i], z[i]), (x[i+1], y[i+1], z[i+1]))
		dis.append(dis_tmp)
		# if dis_tmp >= 0.024:
		# 	ax[0].text(x[i], y[i], str(1))

	# for i in xrange(len(dis)):
	# 	if dis[i] == max(dis):
	# 		print x[i], y[i], z[i]
	# 		print x[i+1], y[i+1], z[i+1]
	# 		ax[0].text(x[i], y[i], str(1))
	# 		ax[0].text(x[i+1], y[i+1], str(1))
	for i in xrange(len(x)-1):
		if dis[i] < 0.012:
			print dis[i]
			ax[0].text(x[i], y[i], str(i))
	# ax.scatter(x_inter, y_inter, s=100, c="red", label="Interpolated points")
	# ax[0].scatter(x_temp_all, y_temp_all, s=50, c="red", label="Downsampled points")
	# ax.scatter(x_down, y_down, c="magenta", s=50, label="Points included due to interpolation needs")
	ax[0].scatter(x, y, s=20, c="orange", label="Processed points")
	ax[0].set_xlabel("x(m)")
	ax[0].set_ylabel("y(m)")
	ax[0].grid()
	ax[0].legend()

	ax[1].set_title("Bar plot of waitlists")
	langs = list(set(down_list))
	freq = []
	for i in set(down_list):
		freq.append(down_list.count(i))
	ax[1].bar(langs, freq)
	plt.xticks(list(set(down_list)))
	plt.yticks(list(set(freq)))
	ax[1].set_xlabel("Number of downsampled points per two consecutive points")
	ax[1].set_ylabel("Frequency")
	ax[1].grid()

	# file = open("./distances.txt", 'a')
	# # plt.savefig("/home/thanasis/Desktop/reactive_yaml_files/figs/" + title)
	
	# file.write(title + " %f, %f\n"%(min(dis), max(dis)))
	# file.close()
	

	# print min(dis), max()	

	plt.show()


main()