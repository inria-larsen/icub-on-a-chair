// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-
/*
* Copyright (C) 2016 INRIA for CODYCO Project
* Author: Serena Ivaldi <serena.ivaldi@inria.fr>
* website: www.codyco.eu
*
* Permission is granted to copy, distribute, and/or modify this program
* under the terms of the GNU General Public License, version 2 or any
* later version published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
* Public License for more details
*/


#include <stdio.h>
#include <iostream>
#include <yarp/os/Network.h>
#include <yarp/dev/ControlBoardInterfaces.h>
#include <yarp/dev/PolyDriver.h>
#include <yarp/dev/IPositionDirect.h>
#include <yarp/os/Time.h>
#include <yarp/sig/Vector.h>
#include <yarp/sig/Matrix.h>

#include <string>
#include <sstream>
#include <fstream>

using namespace yarp::dev;
using namespace yarp::sig;
using namespace yarp::os;
using namespace std;

int nJointsArm=7;
int nJointsTorso=3;
int nJointsLegs=6;
int nbIter;


//---------------------------------------------------------
// open drivers with compliance (real robot)
//---------------------------------------------------------
bool openDriversArm(Property &options, string robot, string part, PolyDriver *&pd, IPositionControl *&ipos, IPositionDirect *&iposd, IEncoders *&ienc, IControlMode2 *&imode, IImpedanceControl *&iimp, ITorqueControl *&itrq)
{
	// open the device drivers
	options.put("device","remote_controlboard");
	options.put("local",string("/upperBodyPlayer/"+part).c_str());
	options.put("remote",string("/"+robot+"/"+part).c_str());
		 
	if(!pd->open(options))
	{
		cout<<"Problems connecting to the remote driver of "<<part<<endl;
		return false;
	}
	if(!pd->isValid())
	{
	    printf("Device not available.  Here are the known devices:\n");
        printf("%s", Drivers::factory().toString().c_str());
        return false;	
	}
	if(!pd->view(imode) || !pd->view(ienc) || !pd->view(ipos) || !(pd->view(iposd)) || !pd->view(iimp) || !pd->view(itrq))
	{
		cout<<"Problems acquiring interfaces for "<<part<<endl;
		return false;
	}
	return true;
}

//---------------------------------------------------------
// open drivers no compliance (simulation)
//---------------------------------------------------------
bool openDriversArm_noImpedance(Property &options, string robot, string part, PolyDriver *&pd, IPositionControl *&ipos, IPositionDirect *&iposd, IEncoders *&ienc, IControlMode2 *&imode)
{
	// open the device drivers
	options.put("device","remote_controlboard");
	options.put("local",string("/upperBodyPlayer/"+part).c_str());
	options.put("remote",string("/"+robot+"/"+part).c_str());

	if(!(pd->open(options)))
	{
		cout<<"Problems connecting to the remote driver of "<<part<<endl;
		return false;
	}
	if(!(pd->isValid()))
	{
	    printf("Device not available.  Here are the known devices:\n");
        printf("%s", Drivers::factory().toString().c_str());
        return false;	
	}
	if(!(pd->view(imode)) || !(pd->view(ienc)) || !(pd->view(ipos)) || !(pd->view(iposd)))
	{
		cout<<"Problems acquiring interfaces for "<<part<<endl;
		return false;
	}
	return true;  
}

//---------------------------------------------------------
// read the trajectory from a file
//---------------------------------------------------------
bool loadFile (string &filename, Matrix &q_RA, Matrix &q_LA, Matrix &q_T, Matrix &timestamps)
{
	cout<<"Reading trajectories from file: "<<filename<<endl;
	
	// open the file
	ifstream inputFile;

	inputFile.open(filename.c_str());
	if (!inputFile.is_open ())
	{
		cout << "ERROR: Can't open file: " << filename << endl;
		return false;
	}
		
	// get the number of lines in the file
	nbIter = 0; string l;
	while (getline (inputFile, l))
	{
		nbIter++;
	}
	cout << "INFO: "<< filename << " is a record of " << nbIter << " iterations" << endl;
		
	inputFile.clear();
	inputFile.seekg(0, ios::beg);
	
	// resizing matrix to get the correct values of the trajectories
	q_RA.resize(nbIter,nJointsArm); q_RA.zero();
	q_LA.resize(nbIter,nJointsArm); q_LA.zero();
	q_T.resize(nbIter,nJointsTorso); q_T.zero();
	timestamps.resize(nbIter,1); timestamps.zero();
	
	Vector base(7);

	// reading the trajectory from the file
	for (int c=0; c<nbIter; c++)
	{
		printf ("Load file %s : \r%d / %d", filename.c_str (), c + 1, nbIter);
		getline (inputFile, l);
		stringstream line;
		line << l;
		//line >> timestamps[c][0];
		
		// the floating base
		line>>base[0];
		line>>base[1];
		line>>base[2];
		line>>base[3];
		line>>base[4];
		line>>base[5];
		line>>base[6];
		
		
		//torso_yaw
		line >> q_T[c][0];
		//l_elbow
		line >> q_LA[c][3];
		//l_wrist_prosup
		line >> q_LA[c][4];
		//l_wrist_yaw
		line >> q_LA[c][6];
		//l_shoulder_pitch
		line >> q_LA[c][0];
		//l_shoulder_roll
		line >> q_LA[c][1];
		//l_shoulder_yaw
		line >> q_LA[c][2];
		//l_wrist_pitch
		line >> q_LA[c][5];
		//r_elbow
		line >> q_RA[c][3];
		//r_wrist_prosup
		line >> q_RA[c][4];
		//r_wrist_yaw
		line >> q_RA[c][6];
		//r_shoulder_pitch
		line >> q_RA[c][0];
		//r_shoulder_roll
		line >> q_RA[c][1];
		//r_shoulder_yaw
		line >> q_RA[c][2];
		//r_wrist_pitch
		line >> q_RA[c][5];
		//torso_pitch
		line >> q_T[c][2];
		//torso_roll
		line >> q_T[c][1];	
	}
	
	cout<<"File is read! "<<endl;
	return true;

}


bool loadFileHumanData (string &filename, Vector &hip_pitch, Vector &hip_roll,  
											Vector &knee, 
											Vector &ankle_pitch, 
											Vector &shoulder_pitch, Vector &shoulder_roll, Vector &shoulder_yaw, 
											Vector &elbow, 
											Vector &torso_pitch)
{
	cout<<"Reading trajectories from file: "<<filename<<endl;
	
	// open the file
	ifstream inputFile;

	inputFile.open(filename.c_str());
	if (!inputFile.is_open ())
	{
		cout << "ERROR: Can't open file: " << filename << endl;
		return false;
	}
		
	// get the number of lines in the file
	nbIter = 0; string l;
	while (getline (inputFile, l))
	{
		nbIter++;
	}
	cout << "INFO: "<< filename << " is a record of " << nbIter << " iterations" << endl;
		
	inputFile.clear();
	inputFile.seekg(0, ios::beg);
	
	// resizing matrix to get the correct values of the trajectories
	hip_pitch.resize(nbIter); 
	hip_roll.resize(nbIter); 
	knee.resize(nbIter); 
	ankle_pitch.resize(nbIter); 
	shoulder_pitch.resize(nbIter); 
	shoulder_roll.resize(nbIter); 
	shoulder_yaw.resize(nbIter); 
	elbow.resize(nbIter); 
	torso_pitch.resize(nbIter);
	
	hip_pitch.zero(); 
	hip_roll.zero();
	knee.zero(); 
	ankle_pitch.zero(); 
	shoulder_pitch.zero();
	shoulder_roll.zero();
	shoulder_yaw.zero(); 
	elbow.zero(); 
	torso_pitch.zero();
	
	double counterToIgnore;

	// reading the trajectory from the file
	for (int c=0; c<nbIter; c++)
	{
		printf ("Load file %s : \r%d / %d", filename.c_str (), c + 1, nbIter);
		getline (inputFile, l);
		stringstream line;
		line << l;
			
		line>>counterToIgnore;
		
		// Frame,0-HipPitch,1-HipRoll,3-Knee,4-AnklePitch,0-ShoulderPitch,1-ShoulderRoll,2-ShoulderYaw,3-Elbow,2-TorsoPitch
		line >> hip_pitch[c];	
		line >> hip_roll[c];
		line >> knee[c];
		line >> ankle_pitch[c];
		line >> shoulder_pitch[c];
		line >> shoulder_roll[c];
		line >> shoulder_yaw[c];
		line >> elbow[c];
		line >> torso_pitch[c];

	}
	
	cout<<"File is read! "<<endl;
	return true;

}

bool startingPointHumanData (Vector &hip_pitch, Vector &hip_roll, Vector &knee, Vector &ankle_pitch, 
							Vector &shoulder_pitch, Vector &shoulder_roll, Vector &shoulder_yaw, 
							Vector &elbow, Vector &torso_pitch,
							Vector &q_RA, Vector &q_LA, Vector &q_T, Vector &q_RL, Vector &q_LL)
{
	int nbIter = hip_pitch.size();
	
    if(hip_pitch.size()<1)
    {
        cout<<"Apparently there is no loaded trajectory... keeping the current point"<<endl;
        return false;
    }
    
    //taking the first element of each trajectory
    
    //torso
    // "torso_yaw" "torso_roll" "torso_pitch"
    q_T[2]=torso_pitch[0];
    
    //arms
    // "l_shoulder_pitch" "l_shoulder_roll" "l_shoulder_yaw" "l_elbow"
    q_RA[0]=shoulder_pitch[0];
    //q_RA[1]=shoulder_roll[0];
    //q_RA[2]=shoulder_yaw[0];
    q_RA[3]=elbow[0];
    q_LA[0]=shoulder_pitch[0];
    //q_LA[1]=shoulder_roll[0];
    //q_LA[2]=shoulder_yaw[0];
    q_LA[3]=elbow[0];
    
    //legs
    // "r_hip_pitch"   "r_hip_roll"    "r_hip_yaw"   "r_knee"  "r_ankle_pitch"  "r_ankle_roll"
    q_LL[0]=hip_pitch[0];
    //q_LL[1]=hip_roll[0];
    q_LL[3]=knee[0];
    q_LL[4]=ankle_pitch[0];
    q_RL[0]=hip_pitch[0];
    //q_RL[1]=hip_roll[0];
    q_RL[3]=knee[0];
    q_RL[4]=ankle_pitch[0];
    
	return true;

}

bool loadHumanDataOnRobotTrajectory(Vector &hip_pitch, Vector &hip_roll, Vector &knee, Vector &ankle_pitch,
                                    Vector &shoulder_pitch, Vector &shoulder_roll, Vector &shoulder_yaw,
                                    Vector &elbow, Vector &torso_pitch,
                                    Vector &q_RA, Vector &q_LA, Vector &q_T, Vector &q_RL, Vector &q_LL,
                                    Matrix &traj_RA, Matrix &traj_LA, Matrix &traj_T, Matrix &traj_RL, Matrix &traj_LL)
{

    int nbIter = hip_pitch.size();
    
    if(hip_pitch.size()<1)
    {
        cout<<"Apparently there is no loaded trajectory... keeping the current point"<<endl;
        return false;
    }

    // resizing matrix to get the correct values of the trajectories
    traj_RA.resize(nbIter,nJointsArm); traj_RA.zero();
    traj_LA.resize(nbIter,nJointsArm); traj_LA.zero();
    traj_T.resize(nbIter,nJointsTorso); traj_T.zero();
    traj_RL.resize(nbIter,nJointsLegs); traj_RL.zero();
    traj_LL.resize(nbIter,nJointsLegs); traj_LL.zero();
    
    // reading the trajectory from the file
    for (int c=0; c<nbIter; c++)
    {
        //first copy the encoders
        for(int j=0; j<nJointsArm; j++)
        {
            traj_RA[c][j]=q_RA[j];
            traj_LA[c][j]=q_LA[j];
        }
        for(int j=0; j<nJointsTorso; j++)
        {
            traj_T[c][j]=q_T[j];
        }
        for(int j=0; j<nJointsLegs; j++)
        {
            traj_RL[c][j]=q_RL[j];
            traj_LL[c][j]=q_LL[j];
        }
        
        //then change the joints from the human data
        
        //torso
        // "torso_yaw" "torso_roll" "torso_pitch"
        traj_T[c][2]=torso_pitch[c];
        
        //arms
        // "l_shoulder_pitch" "l_shoulder_roll" "l_shoulder_yaw" "l_elbow"
        traj_RA[c][0]=shoulder_pitch[c];
        //traj_RA[c][1]=shoulder_roll[c];
        //traj_RA[c][2]=shoulder_yaw[c];
        traj_RA[c][3]=elbow[c];
        traj_LA[c][0]=shoulder_pitch[c];
        //traj_LA[c][1]=shoulder_roll[c];
        //traj_LA[c][2]=shoulder_yaw[c];
        traj_LA[c][3]=elbow[c];
        
        //legs
        // "r_hip_pitch"   "r_hip_roll"    "r_hip_yaw"   "r_knee"  "r_ankle_pitch"  "r_ankle_roll"
        traj_LL[c][0]=hip_pitch[c];
        //traj_LL[c][1]=hip_roll[c];
        traj_LL[c][3]=knee[c];
        traj_LL[c][4]=ankle_pitch[c];
        traj_RL[c][0]=hip_pitch[c];
        //traj_RL[c][1]=hip_roll[c];
        traj_RL[c][3]=knee[c];
        traj_RL[c][4]=ankle_pitch[c];
        
    }
    
    return true;


}
                                    


//---------------------------------------------------------
// check the safety of a trajectory (within the joint limits)
//---------------------------------------------------------
int safety_check_upperbody(Vector &command_RA, Vector &command_LA, Vector &command_T)
{
	int violations=0;
	
	Vector max_RA(7);
	Vector max_LA(7);
	Vector max_T(3);
	Vector min_RA(7);
	Vector min_LA(7);
	Vector min_T(3);
	
	max_RA[0]=6; 	min_RA[0]=-85;
	max_RA[1]=80; 	min_RA[1]=15;
	max_RA[2]=78; 	min_RA[2]=-15;
	max_RA[3]=85; 	min_RA[3]=15;
	max_RA[4]=60; 	min_RA[4]=-70;
	max_RA[5]=0; 	min_RA[5]=-70;
	max_RA[6]=30; 	min_RA[6]=-10;
	
	max_LA[0]=6; 	min_LA[0]=-85;
	max_LA[1]=80; 	min_LA[1]=15;
	max_LA[2]=78; 	min_LA[2]=-15;
	max_LA[3]=85; 	min_LA[3]=15;
	max_LA[4]=60; 	min_LA[4]=-70;
	max_LA[5]=0; 	min_LA[5]=-70;
	max_LA[6]=30; 	min_LA[6]=-10;
	
	max_T[0]=25; 	min_T[0]=-25;
	max_T[1]=8; 	min_T[1]=-8;
	max_T[2]=20; 	min_T[2]=-10;
	
	// arms
	for(int i=0; i<7;i++)
	{
		if(command_RA[i]>max_RA[i]) {  command_RA[i]=max_RA[i];	cout<<"#### max RIGHT_ARM "<<i<<endl; violations++;}
		if(command_RA[i]<min_RA[i]) {  command_RA[i]=min_RA[i];	cout<<"#### min RIGHT_ARM "<<i<<endl; violations++;}
	
		if(command_LA[i]>max_LA[i]) {  command_LA[i]=max_LA[i];	cout<<"#### max LEFT_ARM "<<i<<endl; violations++;}
		if(command_LA[i]<min_LA[i]) {  command_LA[i]=min_LA[i];	cout<<"#### min LEFT_ARM "<<i<<endl; violations++;}
	
	}
	
	// torso
	for(int i=0; i<3;i++)
	{
		if(command_T[i]>max_T[i]) {  command_T[i]=max_T[i];	cout<<"#### max TORSO "<<i<<endl; violations++;}
		if(command_T[i]<min_T[i]) {  command_T[i]=min_T[i];	cout<<"#### min TORSO "<<i<<endl; violations++;}
	
	}

	return violations;
}

int safety_check(Vector &command_RA, Vector &command_LA, Vector &command_T, Vector &command_RL, Vector &command_LL)
{
	int violations=0;
	
	Vector max_RA(7);
	Vector max_LA(7);
	Vector max_T(3);
	Vector max_RL(6);
	Vector max_LL(6);
	Vector min_RA(7);
	Vector min_LA(7);
	Vector min_T(3);
	Vector min_RL(6);
	Vector min_LL(6);	
	
	max_RA[0]=6; 	min_RA[0]=-85;
	max_RA[1]=80; 	min_RA[1]=15;
	max_RA[2]=78; 	min_RA[2]=-15;
	max_RA[3]=85; 	min_RA[3]=15;
	max_RA[4]=60; 	min_RA[4]=-70;
	max_RA[5]=0; 	min_RA[5]=-70;
	max_RA[6]=30; 	min_RA[6]=-10;
	
	max_LA[0]=6; 	min_LA[0]=-85;
	max_LA[1]=80; 	min_LA[1]=15;
	max_LA[2]=78; 	min_LA[2]=-15;
	max_LA[3]=85; 	min_LA[3]=15;
	max_LA[4]=60; 	min_LA[4]=-70;
	max_LA[5]=0; 	min_LA[5]=-70;
	max_LA[6]=30; 	min_LA[6]=-10;
	
	max_T[0]=25; 	min_T[0]=-25;
	max_T[1]=8; 	min_T[1]=-8;
	max_T[2]=20; 	min_T[2]=-10;
	
	max_RL[0]=85; 	min_RL[0]=-30;
	max_RL[1]=80; 	min_RL[1]=  0;
	max_RL[2]=70; 	min_RL[2]=-70;
	max_RL[3]= 0; 	min_RL[3]=-99;
	max_RL[4]=30; 	min_RL[4]=-30;
	max_RL[5]=20; 	min_RL[5]=-20;
	
	max_LL[0]=85; 	min_LL[0]=-30;
	max_LL[1]=80; 	min_LL[1]=  0;
	max_LL[2]=70; 	min_LL[2]=-70;
	max_LL[3]= 0; 	min_LL[3]=-99;
	max_LL[4]=30; 	min_LL[4]=-30;
	max_LL[5]=20; 	min_LL[5]=-20;

	
	// arms
	for(int i=0; i<7;i++)
	{
		if(command_RA[i]>max_RA[i]) {  command_RA[i]=max_RA[i];	cout<<"#### max RIGHT_ARM "<<i<<endl; violations++;}
		if(command_RA[i]<min_RA[i]) {  command_RA[i]=min_RA[i];	cout<<"#### min RIGHT_ARM "<<i<<endl; violations++;}
	
		if(command_LA[i]>max_LA[i]) {  command_LA[i]=max_LA[i];	cout<<"#### max LEFT_ARM "<<i<<endl; violations++;}
		if(command_LA[i]<min_LA[i]) {  command_LA[i]=min_LA[i];	cout<<"#### min LEFT_ARM "<<i<<endl; violations++;}
	
	}
	
	// torso
	for(int i=0; i<3;i++)
	{
		if(command_T[i]>max_T[i]) {  command_T[i]=max_T[i];	cout<<"#### max TORSO "<<i<<endl; violations++;}
		if(command_T[i]<min_T[i]) {  command_T[i]=min_T[i];	cout<<"#### min TORSO "<<i<<endl; violations++;}
	
	}
	
	// legs
	for(int i=0; i<7;i++)
	{
		if(command_RL[i]>max_RL[i]) {  command_RL[i]=max_RL[i];	cout<<"#### max RIGHT_LEG "<<i<<endl; violations++;}
		if(command_RL[i]<min_RL[i]) {  command_RL[i]=min_RL[i];	cout<<"#### min RIGHT_LEG "<<i<<endl; violations++;}
	
		if(command_LL[i]>max_LL[i]) {  command_LL[i]=max_LL[i];	cout<<"#### max LEFT_LEG "<<i<<endl; violations++;}
		if(command_LL[i]<min_LL[i]) {  command_LL[i]=min_LL[i];	cout<<"#### min LEFT_LEG "<<i<<endl; violations++;}
	
	}

	return violations;
}

//==============================================================
//
//		MAIN
//
//==============================================================
int main(int argc, char *argv[]) 
{
		
	//--------------- VARIABLES --------------
	
	int verbosity=1;
	string robotName;
    string fileName;
    int startingPoint=0;
    
    int jointLimitsViolations=0;
    int totalJointsLimitsViolations=0;
    
    // trajectories for the joints from human data
    Vector hip_pitch, hip_roll, knee, ankle_pitch, shoulder_pitch, shoulder_roll, shoulder_yaw, elbow, torso_pitch;
    Matrix q_RA, q_LA, q_T, q_RL, q_LL;
    
    //--------------- CONFIG  --------------
    
	Property params;
    params.fromCommand(argc, argv);
    
    if (params.check("help"))
    {
        cout<<"This module plays a given joints trajectory for the upper body, the trajectory being stored on a file."<<endl
			<<" Usage:   bodyPlayer --robot ROBOTNAME --file FILENAME --verbosity LEVEL --start STARTPOINT"<<endl
			<<" Default values: robot=icubGazeboSim  file=jointAngles_noheader.txt verbosity=2 startpoint=0"<<endl;
        return 1;
    }

    if (!params.check("robot"))
    {
        cout<<"==> Missing robot name, setting default"<<endl;
        robotName="icubGazeboSim";
    }
    else
    {
		robotName=params.find("robot").asString().c_str();
	}
    
     if (!params.check("file"))
    {
        cout<<"==> Missing file name, setting default"<<endl;
        fileName="jointAngles_noheader.txt";
    } 
    else
    {
		fileName=params.find("file").asString().c_str();
	}  
	
	if (!params.check("verbosity"))
    {
        cout<<"==> Missing verbosity, setting default"<<endl;
        verbosity=2;
    } 
    else
    {
		verbosity=params.find("verbosity").asInt();
	}  
	
	if (!params.check("start"))
    {
        cout<<"==> Missing start, setting default"<<endl;
        startingPoint=0;
    } 
    else
    {
		startingPoint=params.find("start").asInt();
		
		if(startingPoint<0) 
		{
			cout<<"Warning: Starting point must be >=0"<<endl;
			startingPoint=0;
		}
	}
    
	if(verbosity>=1)
    cout<<"Robot = "<<robotName<<endl
		<<"File	= "<<fileName<<endl
		<<"Verbosity = "<<verbosity<<endl
		<<"Starting point = "<<startingPoint<<endl;
		
	//--------------- CONFIG  --------------
	
	Network yarp;
    if (!yarp.checkNetwork())
	{
		cout<<"YARP network not available. Aborting."<<endl;
		return -1;
	}
    
    //--------------- READING TRAJECTORY  --------------

	if(!loadFileHumanData(fileName, hip_pitch, hip_roll, knee, ankle_pitch, shoulder_pitch, shoulder_roll, shoulder_yaw, elbow, torso_pitch))
	{
		cout<<"Errors in loading the trajectory file of the human data. Closing."<<endl;
		return -1;
	}
	
	if( startingPoint >= nbIter )
	{
		cout<<"Starting point is after the end of the trajectory. Please choose a starting point smaller than "<<nbIter<<endl;
		return -1;
	}
    
	//--------------- OPENING DRIVERS  --------------
	
	// left arm and leg, right arm and leg, torso
	Property options_LA, options_RA, options_T, options_RL, options_LL;
	PolyDriver *dd_LA, *dd_RA, *dd_T, *dd_RL, *dd_LL;	
	IPositionControl *pos_LA, *pos_RA, *pos_T, *pos_RL, *pos_LL;
	IPositionDirect *posd_LA, *posd_RA, *posd_T, *posd_RL, *posd_LL;
    IEncoders *encs_LA, *encs_RA, *encs_T, *encs_LL, *encs_RL;
    IControlMode2 *ictrl_LA, *ictrl_RA, *ictrl_T, *ictrl_RL, *ictrl_LL;
    IInteractionMode *iint_LA, *iint_RA, *iint_T, *iint_RL, *iint_LL;
    IImpedanceControl *iimp_LA, *iimp_RA, *iimp_T, *iimp_RL, *iimp_LL;
    ITorqueControl *itrq_LA, *itrq_RA, *itrq_T, *itrq_RL, *itrq_LL; 
	
	if(robotName=="icub")
	{	
		dd_LA=new PolyDriver;
		dd_RA=new PolyDriver;
		dd_T=new PolyDriver;
		dd_LL=new PolyDriver;
		dd_RL=new PolyDriver;
		
		if(verbosity>=1)  cout<<"drivers created"<<endl;
	
		if(verbosity>=1) cout<<"** Opening left arm drivers"<<endl;
		if(!openDriversArm(options_LA, robotName, "left_arm", dd_LA, pos_LA, posd_LA, encs_LA, ictrl_LA, iimp_LA, itrq_LA))
		{
			cout<<"Error opening left arm"<<endl;
			return -1;
		}
		
		if(verbosity>=1) cout<<"** Opening right arm drivers"<<endl;
		if(!openDriversArm(options_RA, robotName, "right_arm", dd_RA, pos_RA, posd_RA, encs_RA, ictrl_RA, iimp_RA, itrq_RA))
		{
			cout<<"Error opening right arm"<<endl;
			return -1;
		}
		
		if(verbosity>=1) cout<<"** Opening torso drivers"<<endl;
		if(!openDriversArm(options_T, robotName, "torso", dd_T, pos_T, posd_T, encs_T, ictrl_T, iimp_T, itrq_T))
		{
			cout<<"Error opening torso"<<endl;
			return -1;
		}
		
		if(verbosity>=1) cout<<"** Opening left leg drivers"<<endl;
		if(!openDriversArm(options_LL, robotName, "left_leg", dd_LL, pos_LL, posd_LL, encs_LL, ictrl_LL, iimp_LL, itrq_LL))
		{
			cout<<"Error opening left leg"<<endl;
			return -1;
		}
		
		if(verbosity>=1) cout<<"** Opening right leg drivers"<<endl;
		if(!openDriversArm(options_RL, robotName, "right_leg", dd_RL, pos_RL, posd_RL, encs_RL, ictrl_RL, iimp_RL, itrq_RL))
		{
			cout<<"Error opening right leg"<<endl;
			return -1;
		}
	}
	else
	{

		dd_LA=new PolyDriver;
		dd_RA=new PolyDriver;
		dd_T=new PolyDriver;
		dd_LL=new PolyDriver;
		dd_RL=new PolyDriver;
		
		if(verbosity>=1)  cout<<"drivers created"<<endl;
	
		if(verbosity>=1) cout<<"** Opening left arm drivers"<<endl;
		if(!openDriversArm_noImpedance(options_LA, robotName, "left_arm", dd_LA, pos_LA, posd_LA, encs_LA, ictrl_LA))
		{
			cout<<"Error opening left arm"<<endl;
			return -1;
		}
		
		if(verbosity>=1) cout<<"** Opening right arm drivers"<<endl;
		if(!openDriversArm_noImpedance(options_RA, robotName, "right_arm", dd_RA, pos_RA, posd_RA, encs_RA, ictrl_RA))
		{
			cout<<"Error opening right arm"<<endl;
			return -1;
		}
		
		if(verbosity>=1) cout<<"** Opening torso drivers"<<endl;
		if(!openDriversArm_noImpedance(options_T, robotName, "torso", dd_T, pos_T, posd_T, encs_T, ictrl_T))
		{
			cout<<"Error opening left arm"<<endl;
			return -1;
		}
		
		if(verbosity>=1) cout<<"** Opening left leg drivers"<<endl;
		if(!openDriversArm_noImpedance(options_LL, robotName, "left_leg", dd_LL, pos_LL, posd_LL, encs_LL, ictrl_LL))
		{
			cout<<"Error opening left leg"<<endl;
			return -1;
		}
		
		if(verbosity>=1) cout<<"** Opening right leg drivers"<<endl;
		if(!openDriversArm_noImpedance(options_RL, robotName, "right_leg", dd_RL, pos_RL, posd_RL, encs_RL, ictrl_RL))
		{
			cout<<"Error opening right leg"<<endl;
			return -1;
		}
	}
	
	//---------------  NOW WE CONTROL !! --------------
	
	if(verbosity>=1) cout<< " ***** EVERYTHING IS CREATED ****** "<<endl;
	
	
	//---------------  1) bring to initial position --------------
	
	int i=0;
	int nj_arms=0;
	int nj_torso=0; 
	int nj_legs=0;
	
    pos_RA->getAxes(&nj_arms);
    pos_T->getAxes(&nj_torso);  
    pos_RL->getAxes(&nj_legs);  
    if(verbosity>=1) cout<<"nj arms / torso / legs "<<nj_arms<<" / "<<nj_torso<<" / "<<nj_legs<<endl;
    
    Vector encoders_RA, encoders_LA, encoders_T, encoders_RL, encoders_LL;
    Vector command_RA, command_LA, command_T, command_RL, command_LL;
    Vector tmp;
    
    encoders_RA.resize(nj_arms);
    encoders_LA.resize(nj_arms);
    encoders_T.resize(nj_torso);
    encoders_RL.resize(nj_legs);
    encoders_LL.resize(nj_legs);
    
    command_RA.resize(nj_arms);
    command_LA.resize(nj_arms);
    command_T.resize(nj_torso);
    command_RL.resize(nj_legs);
    command_LL.resize(nj_legs);
    
    // setting accelerations
    for (i=0; i<nj_arms; i++) {command_RA[i]= 50.0; command_LA[i]= 50.0;}
    for (i=0; i<nj_torso; i++) {command_T[i]= 50.0;}  
    for (i=0; i<nj_legs; i++) {command_RL[i]= 50.0; command_LL[i]= 50.0;} 
    pos_RA->setRefAccelerations(command_RA.data());
    pos_LA->setRefAccelerations(command_LA.data());
    pos_T->setRefAccelerations(command_T.data());
    pos_RL->setRefAccelerations(command_RL.data());
    pos_LL->setRefAccelerations(command_LL.data());

	// setting velocities
    for (i = 0; i < nj_arms; i++) 
    {
        command_RA[i] = 5.0;
        command_LA[i] = 5.0;
        pos_RA->setRefSpeed(i, command_RA[i]);
        pos_LA->setRefSpeed(i, command_LA[i]);
    }   
    for(i=0; i<nj_torso; i++)
    {
		command_T[i]=5.0;	
		pos_T->setRefSpeed(i, command_T[i]);
	}
    for (i = 0; i < nj_legs; i++) 
    {
        command_RL[i] = 5.0;
        command_LL[i] = 5.0;
        pos_RL->setRefSpeed(i, command_RL[i]);
        pos_LL->setRefSpeed(i, command_LL[i]);
    }  
	
	// getting the initial configuration of the limbs
	
	if(verbosity>=1) cout<<"Encoders right arm "<<std::endl;
    while(!encs_RA->getEncoders(encoders_RA.data()))
    {
        Time::delay(0.1);
		printf(".");
    }
    if(verbosity>=1) cout<<encoders_RA.toString()<<endl;
    if(verbosity>=1) cout<<"Encoders left arm ";
    while(!encs_LA->getEncoders(encoders_LA.data()))
    {
        Time::delay(0.1);
		printf(".");
    }
    if(verbosity>=1) cout<<encoders_LA.toString()<<endl;
    if(verbosity>=1) cout<<"Encoders torso "<<std::endl;
    while(!encs_T->getEncoders(encoders_T.data()))
    {
        Time::delay(0.1);
		printf(".");
    }
    if(verbosity>=1) cout<<encoders_T.toString()<<endl;
    if(verbosity>=1) cout<<"Encoders right leg "<<std::endl;
    while(!encs_RL->getEncoders(encoders_RL.data()))
    {
        Time::delay(0.1);
		printf(".");
    }
    if(verbosity>=1) cout<<encoders_RL.toString()<<endl;
    if(verbosity>=1) cout<<"Encoders left leg ";
    while(!encs_LL->getEncoders(encoders_LL.data()))
    {
        Time::delay(0.1);
		printf(".");
    }
    if(verbosity>=1) cout<<encoders_LL.toString()<<endl;
    
    command_RA=encoders_RA;
    command_LA=encoders_LA;
    command_T=encoders_T;
    command_RL=encoders_RL;
    command_LL=encoders_LL;
    
    //now set the limbs to the starting point level
    // - only the joints from the human data are changed, the others are fixed
    startingPointHumanData(hip_pitch, hip_roll, knee, ankle_pitch,
                           shoulder_pitch, shoulder_roll, shoulder_yaw,
                           elbow, torso_pitch,
                           command_RA,command_LA, command_T, command_RL, command_LL);
    
    //also load the trajectory
    loadHumanDataOnRobotTrajectory(hip_pitch, hip_roll, knee, ankle_pitch,
                                   shoulder_pitch, shoulder_roll, shoulder_yaw,
                                   elbow, torso_pitch,
                                   encoders_RA, encoders_LA, encoders_T, encoders_RL, encoders_LL,
                                   q_RA, q_LA, q_T, q_RL, q_LL);
    
    jointLimitsViolations = safety_check(command_RA, command_LA, command_T, command_RA, command_LA);
    
    if(jointLimitsViolations==0)
		cout<<" *** FEASIBLE STARTING POSITION *** "<<endl;
	else
		cout<<" *** INFEASIBLE STARTING POSITION *** "<<endl
			<<"\nThe initial position violates the joint limits x"<<jointLimitsViolations<<" times"<<endl
			<<"WE WILL CHANGE THE VALUES"<<endl;
	  
	cout<<"Move the robot to the initial position: "<<endl
		<<" right arm : "<<command_RA.toString()<<endl
		<<" left arm : "<<command_LA.toString()<<endl
		<<" torso : "<<command_T.toString()<<endl
		<<" right leg : "<<command_RL.toString()<<endl
		<<" left leg : "<<command_LL.toString()<<endl
		<<endl
		<<" ==> at starting time = "<<startingPoint<<endl
		<<" ok? (y/n) ";
		
	string chinput;	
	cin >> chinput;
	cout<<endl;
	 
	if(chinput != "y")
	{
		cout << "Closing drivers" << endl;
		if(dd_RA) {delete dd_RA; dd_RA=0; }
		if(dd_LA) {delete dd_LA; dd_LA=0; }
		if(dd_T) {delete dd_T; dd_T=0;}
		if(dd_RL) {delete dd_RL; dd_RL=0; }
		if(dd_LL) {delete dd_LL; dd_LL=0; }
		return 0;		
	}
	
	// set the normal position mode
	for(int j=0; j<nJointsArm; j++) 
	{
		ictrl_LA->setControlMode(j,VOCAB_CM_POSITION);
		ictrl_RA->setControlMode(j,VOCAB_CM_POSITION);
	}
	for(int j=0; j<nJointsTorso; j++) 
		ictrl_T->setControlMode(j,VOCAB_CM_POSITION);
	for(int j=0; j<nJointsLegs; j++) 
	{
		ictrl_LL->setControlMode(j,VOCAB_CM_POSITION);
		ictrl_RL->setControlMode(j,VOCAB_CM_POSITION);
	}
		
	cout<<" Moving right and left arm "<<endl;
    pos_RA->positionMove(command_RA.data());
    pos_LA->positionMove(command_LA.data());
    Time::delay(3.0);
    cout<<" Moving right and left leg "<<endl;
    pos_RL->positionMove(command_RL.data());
    pos_LL->positionMove(command_LL.data());
    Time::delay(0.2);
    cout<<" Moving torso "<<endl;
    pos_T->positionMove(command_T.data());
    Time::delay(3.0);

    
    
    cout<<" Starting movement ? (y/n) ";
	cin >> chinput; 
	cout<<endl;
	
	if(chinput != "y")
	{
		cout << "Closing drivers" << endl;
		if(dd_RA) {delete dd_RA; dd_RA=0; }
		if(dd_LA) {delete dd_LA; dd_LA=0; }
		if(dd_T) {delete dd_T; dd_T=0;}
		if(dd_RL) {delete dd_RL; dd_RL=0; }
		if(dd_LL) {delete dd_LL; dd_LL=0; }
		return 0;		
	}
	
	
	//---------------  2) play trajectory --------------
	
	cout<<"******  MOVING! ****** "<<endl;
	
	Time::delay(1.0);
	bool cmode=true;
	bool notpossible=false;

/*	
	//first set the direct position mode
	for(int j=0; j<nJointsArm; j++) 
	{
		cmode=ictrl_LA->setControlMode(j,VOCAB_CM_POSITION_DIRECT);
		
		if(cmode==false)
		{
			ictrl_LA->setControlMode(j,VOCAB_CM_POSITION);
			cout<<"Left arm: joint "<<j<<" cannot change direct position"<<endl;
			notpossible=true;
		}
				
		cmode=ictrl_RA->setControlMode(j,VOCAB_CM_POSITION_DIRECT);
		
		if(cmode==false)
		{
			ictrl_RA->setControlMode(j,VOCAB_CM_POSITION);
			cout<<"Right arm: joint "<<j<<" cannot change direct position"<<endl;
			notpossible=true;
		}
			
	}
	for(int j=0; j<nJointsTorso; j++) 
	{
			cmode=ictrl_T->setControlMode(j,VOCAB_CM_POSITION_DIRECT);	
			
			if(cmode==false)
			{
				ictrl_T->setControlMode(j,VOCAB_CM_POSITION);
				cout<<"Torso: joint "<<j<<" cannot change direct position"<<endl;
				notpossible=true;
			}
			
	}
		
		//// if there is errors in the direct mode, do not play the trajectory
		
		if(notpossible == true)
		{
			cout << "Closing drivers" << endl;
			if(dd_RA) {delete dd_RA; dd_RA=0; }
			if(dd_LA) {delete dd_LA; dd_LA=0; }
			if(dd_T) {delete dd_T; dd_T=0;}
			return 0;		
		}
		else
		{
			cout<<"**** direct position possible! ****"<<endl;
		}
	*/
		
		//// ok you can play
	/*
	for(int t=startingPoint; t<nbIter; t++)
	{
		for(i=0; i<nJointsArm; i++) command_RA[i] = q_RA[t][i];
		for(i=0; i<nJointsArm; i++) command_LA[i] = q_LA[t][i];
		for(i=0; i<nJointsTorso; i++) command_T[i] = q_T[t][i];
		
		jointLimitsViolations = safety_check(command_RA, command_LA, command_T);
		totalJointsLimitsViolations += jointLimitsViolations;
		
		if(verbosity>=1)   printf ("Moving : \r%d / %d  - violating %d", t, nbIter, jointLimitsViolations);
    	
		pos_T->positionMove(command_T.data());
		pos_RA->positionMove(command_RA.data());
		pos_LA->positionMove(command_LA.data());
    
		//posd_T->setPositions(command_T.data());
		//posd_RA->setPositions(command_RA.data());
		//posd_LA->setPositions(command_LA.data());
	
		//10ms
		Time::delay(0.01);
		//1ms 
		//Time::delay(0.001);
		
	}*/
	
	for(int t=startingPoint; t<nbIter; t+=1)
	{
		for(i=0; i<nJointsArm; i++)   command_RA[i] = q_RA[t][i];
		for(i=0; i<nJointsArm; i++)   command_LA[i] = q_LA[t][i];
		for(i=0; i<nJointsTorso; i++) command_T[i] = q_T[t][i];
        for(i=0; i<nJointsLegs; i++)  command_RL[i] = q_RL[t][i];
        for(i=0; i<nJointsLegs; i++)  command_LL[i] = q_LL[t][i];
		
		jointLimitsViolations = safety_check(command_RA, command_LA, command_T, command_RL, command_LL);
		totalJointsLimitsViolations += jointLimitsViolations;
		
		if(verbosity>=1)   printf ("Moving : \r%d / %d  - violating %d", t, nbIter, jointLimitsViolations);
    	
		pos_T->positionMove(command_T.data());
		pos_RA->positionMove(command_RA.data());
		pos_LA->positionMove(command_LA.data());
		pos_RL->positionMove(command_RL.data());
		pos_LL->positionMove(command_LL.data());
    
		//100ms
		Time::delay(0.1);
		//10ms
		//Time::delay(0.01);
		//1ms 
		//Time::delay(0.001);
		
	}
	
	Time::delay(1.0);
	
	cout<<"\n******  FINISHED! ****** "<<endl
		<<"\nYou violated the joints limits x"<<totalJointsLimitsViolations<<" times"<<endl;;

/*	
	//go back to a normal position mode
	for(int j=0; j<nJointsArm; j++) 
	{
		ictrl_LA->setControlMode(j,VOCAB_CM_POSITION);
		ictrl_RA->setControlMode(j,VOCAB_CM_POSITION);
	}
	for(int j=0; j<nJointsTorso; j++) 
		ictrl_T->setControlMode(j,VOCAB_CM_POSITION);	
*/
	
	
	//---------------  CLOSING --------------


	if(verbosity>=1) cout << "Closing drivers" << endl;

	if(dd_RA) {delete dd_RA; dd_RA=0; }
	if(dd_LA) {delete dd_LA; dd_LA=0; }
	if(dd_T) {delete dd_T; dd_T=0;}
    if(dd_RL) {delete dd_RL; dd_RL=0; }
    if(dd_LL) {delete dd_LL; dd_LL=0; }

	return 0;
}
	
