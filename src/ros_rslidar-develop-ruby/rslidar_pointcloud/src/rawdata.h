/*
 *	Copyright (C) 2018-2020 Robosense Authors
 *  License: Modified BSD Software License Agreement
 *
 *  $Id$
 */

/** @file
 *
 *  @brief Interfaces for interpreting raw packets from the Robosense 3D LIDAR.
 */

#ifndef _RAWDATA_H
#define _RAWDATA_H

#include <ros/ros.h>
#include <ros/package.h>
#include <rslidar_msgs/rslidarPacket.h>
#include <rslidar_msgs/rslidarScan.h>
#include <std_msgs/Float32.h>
#include "std_msgs/String.h"
#include <pcl/point_types.h>
#include <pcl_ros/point_cloud.h>
#include <pcl_ros/impl/transforms.hpp>
#include <pcl_conversions/pcl_conversions.h>
#include <stdio.h>
namespace rslidar_rawdata
{
#define RS_TO_RADS(x) ((x) * (M_PI) / 180)

// static const float  ROTATION_SOLUTION_ = 0.18f;  //水平角分辨率 10hz
static const int SIZE_BLOCK = 100;
static const int RAW_SCAN_SIZE = 3;
static const int SCANS_PER_BLOCK = 128;
static const int BLOCK_DATA_SIZE_RS128 = (SCANS_PER_BLOCK * RAW_SCAN_SIZE);  // 384

static const float ROTATION_RESOLUTION = 0.01f;   /**< degrees 旋转角分辨率*/
static const uint16_t ROTATION_MAX_UNITS = 36000; /**< hundredths of degrees */

static const float DISTANCE_MAX = 200.0f;            /**< meters */
static const float DISTANCE_MIN = 0.5f;              /**< meters */
static const float DISTANCE_RESOLUTION = 0.005f;      /**< meters */
static const float DISTANCE_MAX_UNITS = (DISTANCE_MAX / DISTANCE_RESOLUTION + 1.0f);
/** @todo make this work for both big and little-endian machines */
static const uint16_t UPPER_BANK = 0xeeff;  //
static const uint16_t LOWER_BANK = 0xddff;

/** Special Defines for RS128 support **/
static const float RS128_DSR_TOFFSET = 3.23f;       // [µs]
static const float RS128_BLOCK_TDURATION = 55.55f;  // [µs]

static const int TEMPERATURE_MIN = 31;

/** \brief Raw rslidar data block.
 *
 *  Each block contains data from either the upper or lower laser
 *  bank.  The device returns three times as many upper bank blocks.
 *
 *  use stdint.h types, so things work with both 64 and 32-bit machines
 */
// block
typedef struct raw_block_rs128
{
  uint8_t header;
  uint8_t retWaveId;
  uint8_t rotation_1;
  uint8_t rotation_2;  /// combine rotation1 and rotation2 together to get 0-35999, divide by 100 to get degrees
  uint8_t data[BLOCK_DATA_SIZE_RS128];  // 384
} raw_block_rs128;

/** used for unpacking the first two data bytes in a block
 *
 *  They are packed into the actual data stream misaligned.  I doubt
 *  this works on big endian machines.
 */
union two_bytes
{
  uint16_t uint;
  uint8_t bytes[2];
};

static const int PACKET_SIZE = 1248;
static const int BLOCKS_PER_PACKET_RS128 = 3;
static const int PACKET_STATUS_SIZE = 4;
static const int SCANS_PER_PACKET = (SCANS_PER_BLOCK * BLOCKS_PER_PACKET_RS128);

/** \brief Raw Rsldar packet.
 *
 *  revolution is described in the device manual as incrementing
 *    (mod 65536) for each physical turn of the device.  Our device
 *    seems to alternate between two different values every third
 *    packet.  One value increases, the other decreases.
 *
 *  \todo figure out if revolution is only present for one of the
 *  two types of status fields
 *
 *  status has either a temperature encoding or the microcode level
 */

typedef struct raw_packet_rs128
{
  raw_block_rs128 blocks[BLOCKS_PER_PACKET_RS128];
  uint16_t revolution;
  uint8_t status[PACKET_STATUS_SIZE];
} raw_packet_rs128;


/** \brief RSLIDAR data conversion class */
class RawData
{
public:
  RawData();

  ~RawData()
  {
    this->cos_lookup_table_.clear();
    this->sin_lookup_table_.clear();
  }

  /*load the cablibrated files: angle, distance, intensity*/
  void loadConfigFile(ros::NodeHandle node, ros::NodeHandle private_nh);

  /*unpack the RS16 UDP packet and opuput PCL PointXYZI type*/
  void unpack(const rslidar_msgs::rslidarPacket& pkt, pcl::PointCloud<pcl::PointXYZI>::Ptr pointcloud);

  /*unpack the RS128 UDP packet and opuput PCL PointXYZI type*/
  void unpack_RS128(const rslidar_msgs::rslidarPacket& pkt, pcl::PointCloud<pcl::PointXYZI>::Ptr pointcloud);

  /*compute temperature*/
  float computeTemperature128(unsigned char bit2, unsigned char bit1);

  /*estimate temperature*/
  int estimateTemperature(float Temper);

  /*calibrated the disctance*/
  float pixelToDistance(int pixelValue, int passageway);

  /*calibrated the azimuth*/
  unsigned int correctAzimuth(float azimuth_f, int passageway);

  void processDifop(const rslidar_msgs::rslidarPacket::ConstPtr& difop_msg);
  ros::Subscriber difop_sub_;
  ros::Publisher temperature_pub_;
  bool is_init_angle_;
  int block_num = 0;

private:
  float Rx_;  // the optical center position in the lidar coordination in x direction
  float Ry_;  // the optical center position in the lidar coordination in y direction, for now not used
  float Rz_;  // the optical center position in the lidar coordination in z direction
  bool angle_flag_;
  float start_angle_;
  float end_angle_;
  float max_distance_;
  float min_distance_;
  int return_mode_;
  bool info_print_flag_;
  /* cos/sin lookup table */
  std::vector<double> cos_lookup_table_;
  std::vector<double> sin_lookup_table_;
};

static int VERT_ANGLE[128];
static int HORI_ANGLE[128];
static int g_ChannelNum[128][51];

static float temper = 31.0;
static int tempPacketNum = 0;
static int numOfLasers = 128;
static int TEMPERATURE_RANGE = 40;
static float lastAzimuth = -1;
static float azimuthDiff = 10;

}  // namespace rslidar_rawdata

#endif  // __RAWDATA_H