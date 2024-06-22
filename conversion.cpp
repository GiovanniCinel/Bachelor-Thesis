#include <ros/ros.h>
#include <rosbag/view.h>
#include <drapebot_msgs/SkeletonQuaternion.h>

#include <dirent.h>     
#include <sys/stat.h>
#include <gtest/gtest.h>

#include <sstream>
#include <iostream>
#include <fstream>

#include <vector>
#include <string>

#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH

//Struct used to store the data of one joint
struct Pose {
    int segment_id;

    double position_x;
    double position_y;
    double position_z;

    double orientation_x;
    double orientation_y;
    double orientation_z;
    double orientation_w;
};

int main ()
{
    rosbag::Bag bag;
    std::string path;
    std::string directory_path;
    std::string file_name;
    std::vector<std::string> bag_files; 
    Pose pose;

    std::cout << "Insert the path of the folder containing the files to convert, starting from the /home directory" << std::endl;
    std::cin >> directory_path;
    
    // Pointer to directory
    DIR *dir;

    // Pointer to directory entry
    struct dirent *ent;

    // Structure for file status
    struct stat st;                 
    
    if ((dir = opendir(directory_path.c_str())) != NULL) {

        // Read all files in the folder one at a timer 
        while ((ent = readdir(dir)) != NULL) {

            // Create full file path
            std::string file_path = directory_path + ent->d_name;   
            
            // Get file status
            stat(file_path.c_str(), &st);                

            // Check if the entry is a regular file
            if (S_ISREG(st.st_mode)) {   

                // Add file name to vector             
                bag_files.push_back(ent->d_name);           
            } 
        }
        
        // Close the directory pointer
        closedir(dir);        
    }
    else {
        std::cout << "Could not open directory" << std::endl;
        return 1;
    }

    

    while(!bag_files.empty()){ 
        std::string input;
        std::stringstream ss(input);
        std::string line;
        std::string clean;
        std::string temp;
        std::string first_line = "timestamp,";
        
        int timestamp = 0;
        double start = 0; 
        
        file_name = bag_files.at(bag_files.size()-1);
        bag_files.pop_back();
        
        path = directory_path + file_name;
        file_name = file_name.substr(0,13) + "csv";
        
        std::cout << "Start conversion to: ";
        std::cout << file_name << std::endl;
        
        bag.open(path, rosbag::bagmode::Read);     
        rosbag::View view(bag);
        std::ofstream file(file_name);

        if(!file.is_open()){
            std::cerr << "Error, file not open!" << std::endl;
            return 1;
        }

        // Define first line of the .csv file containing fields name
        for(int i = 1; i <= 23; i++ ){
            first_line = first_line +
            std::to_string(i) + "_position_x," + 
            std::to_string(i) + "_position_y," +  
            std::to_string(i) + "_position_z," +
            std::to_string(i) + "_orientation_x," +
            std::to_string(i) + "_orientation_y," +
            std::to_string(i) + "_orientation_z," +
            std::to_string(i) + "_orientation_w" ;

            if(i != 23)
                first_line = first_line + ",";
        }
        
        file << first_line << std::endl;

        // Scans all the timestamp
        foreach(rosbag::MessageInstance const m, view)
        {

            drapebot_msgs::SkeletonQuaternion::ConstPtr s = m.instantiate<drapebot_msgs::SkeletonQuaternion>();

            if (s != NULL){

                // Save time in seconds to temp_time
                double temp_time = s->header.stamp.sec + s->header.stamp.nsec * 1e-9;

                if (timestamp == 0) {
                    // Save start time in seconds to start
                    start = temp_time;
                }

                temp_time = temp_time - start;
                
                // Time in milliseconds
                temp = std::to_string((int)(temp_time*1000));
                
                //Scans all the joint corresponding to a timestamp
                for(int i = 0; i < s->skeleton.size(); i++){

                    ss << s->skeleton[i];
                    
                    //Extract joint position and orientation data 
                    while (getline(ss, line)) {
                        if (line.find("segment_id:") != std::string::npos) {
                            pose.segment_id = stoi(line.substr(line.find(":")+2));
                        }
                        if (line.find("position:") != std::string::npos) {
                            getline(ss, line);
                            pose.position_x = std::stod(line.substr(line.find(":")+2));
                            getline(ss, line);
                            pose.position_y = std::stod(line.substr(line.find(":")+2));
                            getline(ss, line);
                            pose.position_z = std::stod(line.substr(line.find(":")+2));
                        }
                        if (line.find("orientation:") != std::string::npos) {
                            getline(ss, line);
                            pose.orientation_x = std::stod(line.substr(line.find(":")+2));
                            getline(ss, line);
                            pose.orientation_y = std::stod(line.substr(line.find(":")+2));
                            getline(ss, line);
                            pose.orientation_z = std::stod(line.substr(line.find(":")+2));
                            getline(ss, line);
                            pose.orientation_w = std::stod(line.substr(line.find(":")+2));
                        }
                    }
                    
                    temp = temp + "," +
                    std::to_string(pose.position_x) + "," + 
                    std::to_string(pose.position_y) + "," + 
                    std::to_string(pose.position_z) + "," +  
                    std::to_string(pose.orientation_x) + "," + 
                    std::to_string(pose.orientation_y) + "," + 
                    std::to_string(pose.orientation_z) + "," + 
                    std::to_string(pose.orientation_w);

                }

                file << temp << std::endl;
                temp = "";
                timestamp++;
                ss.clear();

            }
        }
       
        file.close();
        bag.close();
    }

 return(0);
}
