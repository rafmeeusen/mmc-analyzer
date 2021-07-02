#ifndef MMC_ANALYZER_H
#define MMC_ANALYZER_H

#include <Analyzer.h>
#include "mmcAnalyzerResults.h"
#include "mmcSimulationDataGenerator.h"

class mmcAnalyzerSettings;
class ANALYZER_EXPORT mmcAnalyzer : public Analyzer2
{
public:
	mmcAnalyzer();
	virtual ~mmcAnalyzer();

	virtual void SetupResults();
	virtual void WorkerThread();

	virtual U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels );
	virtual U32 GetMinimumSampleRateHz();

	virtual const char* GetAnalyzerName() const;
	virtual bool NeedsRerun();

protected: //functions
    void ReadAndMarkCmdBits(U8 nrOfBits);
    bool GetCommandBit();


protected: //vars
	std::auto_ptr< mmcAnalyzerSettings > mSettings;
	std::auto_ptr< mmcAnalyzerResults > mResults;
	AnalyzerChannelData* mClock;
	AnalyzerChannelData* mCommand;

	mmcSimulationDataGenerator mSimulationDataGenerator;
	bool mSimulationInitilized;

	//Serial analysis vars:
	U32 mSampleRateHz;
	U32 mStartOfStopBitOffset;
	U32 mEndOfStopBitOffset;
};

extern "C" ANALYZER_EXPORT const char* __cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer* __cdecl CreateAnalyzer( );
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer( Analyzer* analyzer );

#endif //MMC_ANALYZER_H
