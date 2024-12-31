#include <ros/ros.h>
#include <rosbag/bag.h>
#include <rosbag/view.h>
#include <rosbag/query.h>
#include <boost/foreach.hpp>
#include <boost/progress.hpp>

class TimeBasedFilter {
public:
    bool filterBag(const std::string& inbag_path, 
                  const std::string& outbag_path,
                  const double start_offset,  // 相对于起始时间的偏移量(秒)
                  const double end_offset) {  // 相对于结束时间的偏移量(秒)
        try {
            rosbag::Bag inbag, outbag;
            
            // 打开输入bag
            inbag.open(inbag_path, rosbag::bagmode::Read);
            if (!inbag.isOpen()) {
                ROS_ERROR("Failed to open input bag!");
                return false;
            }

            // 检查索引
            // if (!inbag.isIndexed()) {
            //     ROS_WARN("Bag file is not indexed. Rebuilding index...");
            //     try {
            //         inbag.close();
            //         rosbag::Bag::reindex(inbag_path);
            //         inbag.open(inbag_path, rosbag::bagmode::Read);
            //     } catch (rosbag::BagException& e) {
            //         ROS_ERROR("Failed to rebuild index: %s", e.what());
            //         return false;
            //     }
            // }

            // 获取bag的时间范围
            rosbag::View view_all(inbag);
            ros::Time bag_start = view_all.getBeginTime();
            ros::Time bag_end = view_all.getEndTime();
            double total_duration = (bag_end - bag_start).toSec();
            
            // 计算目标时间范围
            ros::Time target_start = bag_start + ros::Duration(start_offset);
            ros::Time target_end = bag_end - ros::Duration(end_offset);

            // 检查时间范围是否有效
            if (target_start > bag_end || target_end < bag_start || target_end < target_start) {
                ROS_INFO("Invalid time range:");
                ROS_INFO("  Bag duration: %.2f seconds", total_duration);
                ROS_INFO("  Start offset: %.2f seconds from beginning", start_offset);
                ROS_INFO("  End offset: %.2f seconds from end", end_offset);
                ROS_INFO("  Resulting range: %.2f to %.2f seconds",
                         (target_start - bag_start).toSec(),
                         total_duration - end_offset);
                return true;
            }

            // 创建时间查询视图
            rosbag::View view(inbag, ros::Time(target_start), ros::Time(target_end));

            // 检查是否有符合条件的消息
            if (view.size() == 0) {
                ROS_INFO("No messages found in the specified time range");
                ROS_INFO("Time range: %.2f to %.2f seconds of total %.2f seconds",
                         (target_start - bag_start).toSec(),
                         total_duration - end_offset,
                         total_duration);
                return true;
            }

            // 打开输出bag
            outbag.open(outbag_path, rosbag::bagmode::Write);
            outbag.setCompression(rosbag::compression::LZ4);
            
            // 设置缓冲区
            // const size_t BUFFER_SIZE = 64 * 1024 * 1024;  // 64MB
            // inbag.setBufferSize(BUFFER_SIZE);
            // outbag.setBufferSize(BUFFER_SIZE);

            // 显示处理信息
            ROS_INFO("Processing messages:");
            ROS_INFO("  Time range: %.2f to %.2f seconds of total %.2f seconds",
                     (target_start - bag_start).toSec(),
                     total_duration - end_offset,
                     total_duration);
            ROS_INFO("  Message count: %lu", view.size());

            // 显示进度条
            boost::progress_display progress(view.size());

            // 处理消息
            for (const rosbag::MessageInstance& msg : view) {
                outbag.write(msg.getTopic(), msg.getTime(), msg);
                ++progress;
            }

            // 关闭文件
            inbag.close();
            outbag.close();

            return true;
        }
        catch (rosbag::BagException& e) {
            ROS_ERROR("Bag processing error: %s", e.what());
            return false;
        }
    }

    // 打印bag信息
    void printBagInfo(const std::string& bag_path) {
        rosbag::Bag bag(bag_path, rosbag::bagmode::Read);
        rosbag::View view(bag);
        
        ros::Time start_time = view.getBeginTime();
        ros::Time end_time = view.getEndTime();
        double duration = (end_time - start_time).toSec();
        
        ROS_INFO("Bag information:");
        ROS_INFO("  Duration: %.2f seconds", duration);
        ROS_INFO("  Start time: %.2f", start_time.toSec());
        ROS_INFO("  End time: %.2f", end_time.toSec());
        ROS_INFO("  Message count: %lu", view.size());
        
        // 统计话题信息
        std::map<std::string, int> topic_message_count;
        BOOST_FOREACH(rosbag::MessageInstance const m, view) {
            topic_message_count[m.getTopic()]++;
        }
        
        ROS_INFO("Topics:");
        for (const auto& topic : topic_message_count) {
            ROS_INFO("  %s: %d messages (%.1f msg/sec)", 
                     topic.first.c_str(), 
                     topic.second,
                     topic.second / duration);
        }
        
        bag.close();
    }
};

int main(int argc, char** argv) {
    ros::init(argc, argv, "time_based_filter");
    
    if (argc != 5) {
        ROS_ERROR("Usage: %s input.bag output.bag start_offset end_offset", argv[0]);
        ROS_ERROR("  start_offset: time offset in seconds from the beginning");
        ROS_ERROR("  end_offset: time offset in seconds from the end");
        ROS_ERROR("Example: %s input.bag output.bag 10.0 5.0", argv[0]);
        ROS_ERROR("  This will extract data from 10 seconds after start to 5 seconds before end");
        return 1;
    }
    
    std::string input_bag = argv[1];
    std::string output_bag = argv[2];
    double start_offset = std::stod(argv[3]);
    double end_offset = std::stod(argv[4]);
    
    TimeBasedFilter filter;
    
    // 打印输入bag信息
    ROS_INFO("Input bag information:");
    filter.printBagInfo(input_bag);
    
    // 执行过滤
    if (filter.filterBag(input_bag, output_bag, start_offset, end_offset)) {
        ROS_INFO("Filtering completed successfully");
        
        // 打印输出bag信息
        ROS_INFO("\nOutput bag information:");
        filter.printBagInfo(output_bag);
    } else {
        ROS_ERROR("Filtering failed");
        return 1;
    }
    
    return 0;
}
