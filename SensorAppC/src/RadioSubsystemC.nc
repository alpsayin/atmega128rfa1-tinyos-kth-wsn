
#include "RadioConfig.h"
#include "packet_types.h"

configuration RadioSubsystemC
{
	provides 
	{
		//these interfaces should be used by Controller to send packets/receive commands
		//if a root is going to be set, it must be set with RootControl before Init
		interface RootControl as RadioSubsystemRootControl;
		interface Init as RadioSubsystemInit;
		interface SetNow<data_packet_t> as SetRadioData;
		interface SetNow<command_packet_t> as SetRadioCommand;
		interface SetNow<status_packet_t> as SetRadioStatus;
		interface Notify<command_packet_t> as NotifyRadioCommand;
	}
	uses
	{
		//these interfaces should be wired to SerialPacketForwarderC
		//they shouldn't be used by controller
		interface Set<data_packet_t> as ForwardData;
		interface Set<status_packet_t> as ForwardStatus;
		interface Set<command_packet_t> as ForwardCommand;
		interface Notify<command_packet_t> as NotifySerialCommand;
	}
}
implementation
{
	components RadioSubsystemP;
	
	RadioSubsystemInit = RadioSubsystemP.RadioSubsystemInit;
	SetRadioData = RadioSubsystemP.SetRadioData;
	SetRadioStatus = RadioSubsystemP.SetRadioStatus;
	SetRadioCommand = RadioSubsystemP.SetRadioCommand;
	NotifyRadioCommand = RadioSubsystemP.NotifyRadioCommand;
	
	ForwardData = RadioSubsystemP.ForwardData;
	ForwardStatus = RadioSubsystemP.ForwardStatus;
	ForwardCommand = RadioSubsystemP.ForwardCommand;
	NotifySerialCommand = RadioSubsystemP.NotifySerialCommand;
	
	//components MainC;
	//MainC.SoftwareInit -> RadioSubsystemP.RadioSubsystemInit;
	
	components LedsC;
  	RadioSubsystemP.Leds -> LedsC;

	components DisseminationC;
	RadioSubsystemP.DisseminationControl -> DisseminationC;
	  
	components new DisseminatorC(command_packet_t, 0xaa) as DissCommand;
	RadioSubsystemP.CommandValue -> DissCommand;
	RadioSubsystemP.CommandUpdate -> DissCommand;
	  
	components CollectionC as Collector;
	RadioSubsystemP.RoutingControl -> Collector;
	RadioSubsystemRootControl = Collector.RootControl;
	RadioSubsystemP.DataCollectionReceive -> Collector.Receive[0xbb];
	RadioSubsystemP.StatusCollectionReceive -> Collector.Receive[0xcc];
	RadioSubsystemP.CommandCollectionReceive -> Collector.Receive[0xdd];
	RadioSubsystemP.HistoryCollectionReceive -> Collector.Receive[0xee];
  
	components new CollectionSenderC(0xbb) as DataCollectionSender;
	RadioSubsystemP.DataCollectionSend -> DataCollectionSender;
	
	components new CollectionSenderC(0xcc) as StatusCollectionSender;
	RadioSubsystemP.StatusCollectionSend -> StatusCollectionSender;
	
	components new CollectionSenderC(0xdd) as CommandCollectionSender;
	RadioSubsystemP.CommandCollectionSend -> CommandCollectionSender;
	
	components new CollectionSenderC(0xee) as HistoryCollectionSender;
	RadioSubsystemP.HistoryCollectionSend -> HistoryCollectionSender;
	
}