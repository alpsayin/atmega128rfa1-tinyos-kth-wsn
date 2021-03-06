//
//
//  ControllerC.nc
//
//  aa
//
//  Created by Tian Junzhe on 12/9/11.
//  Copyright 2011 KTH. All rights reserved.
//
//	zn
//
//


#include "ControllerConfig.h"

configuration ControllerC{

	uses interface Boot;
	provides interface Init;
	//----------------------Connect to SensorC------------------------------
	provides {
		interface Set<uint8_t> as SPEnable;
		interface Notify<status_packet_t>;	
	}
	uses {
		interface Init as InitSensorC;
		interface Get<status_packet_t> as GetStatus;
		interface ArrayPipe<data_packet_t> as GetData;
		interface Read<data_packet_t> as GetDataOne;
	}
	//----------------------Connect to RadioSubsystem-----------------------
	uses 
	{
		//these interfaces should be used by Controller to send packets/receive commands
		//if a root is going to be set, it must be set with RootControl before Init
		interface RootControl as RadioSubsystemRootControl;
		interface Init as RadioSubsystemInit;
		interface ArrayPipe<history_packet_t> as SetRadioHistory;
		interface SetNow<data_packet_t> as SetRadioData;
		interface SetNow<command_packet_t> as SetRadioCommand;
		interface SetNow<status_packet_t> as SetRadioStatus;
		interface Notify<command_packet_t> as NotifyRadioCommand;
	}
	//----------------------Connect to SerialPacketForwarder----------------
	uses
	{
		interface Init as SerialPacketForwarderInit;
		interface StdControl as SerialPacketForwarderControl;
	}
}
implementation{
	
	components ControllerP;
	components IOInterfaceC;
	components LedsC;
	components new PacketTypesP();	
	
	Boot		= ControllerP.Boot;
	
	Init		= ControllerP.Init;
	InitSensorC = ControllerP.InitSensorC;
	
	SPEnable	= ControllerP.SPEnable;
	GetStatus	= ControllerP.GetStatus;
	GetData		= ControllerP.GetData;
	GetDataOne	= ControllerP.GetDataOne;
	Notify		= ControllerP.SensorNotification;


	SerialPacketForwarderControl= ControllerP.SerialPacketForwarderControl;
	RadioSubsystemRootControl	= ControllerP.RadioSubsystemRootControl;
	RadioSubsystemInit			= ControllerP.RadioSubsystemInit;
	SerialPacketForwarderInit	= ControllerP.SerialPacketForwarderInit;
	SetRadioHistory				= ControllerP.SetRadioHistory;
	SetRadioStatus				= ControllerP.SetRadioStatus;
	SetRadioData				= ControllerP.SetRadioData;
	SetRadioCommand				= ControllerP.SetRadioCommand;
	NotifyRadioCommand			= ControllerP.NotifyRadioCommand;
	
	
	ControllerP.PacketTypes			-> PacketTypesP;
	ControllerP.Leds				-> LedsC;
	ControllerP.CheckRoot			-> IOInterfaceC.CheckRoot;
	ControllerP.LightSensorEnable	-> IOInterfaceC.LightSensorEnable;
	ControllerP.HumiditySensorEnable-> IOInterfaceC.HumiditySensorEnable;
	ControllerP.PressureSensorEnable-> IOInterfaceC.PressureSensorEnable;
	ControllerP.VoltageSensorEnable -> IOInterfaceC.VoltageSensorEnable;

}