#ifndef MMC_SIMULATION_DATA_GENERATOR
#define MMC_SIMULATION_DATA_GENERATOR

#include <SimulationChannelDescriptor.h>
#include <string>
class mmcAnalyzerSettings;

class mmcSimulationDataGenerator
{
public:
	mmcSimulationDataGenerator();
	~mmcSimulationDataGenerator();

	void Initialize( U32 simulation_sample_rate, mmcAnalyzerSettings* settings );
	U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channel );

protected:
	mmcAnalyzerSettings* mSettings;
	U32 mSimulationSampleRateHz;

protected:
	void CreateSerialByte();
	std::string mSerialText;
	U32 mStringIndex;

	SimulationChannelDescriptor mSerialSimulationData;

};
#endif //MMC_SIMULATION_DATA_GENERATOR