#ifndef MMC_ANALYZER_H
#define MMC_ANALYZER_H

#include <Analyzer.h>
#include "mmcAnalyzerResults.h"
#include "mmcSimulationDataGenerator.h"

#include <vector>
using namespace std;

#define MMC_CMD_BITLEN      48

typedef struct _PacketType {
    bool dir;
    U16  bitlen;
} PacketType ;

typedef struct _RawBits {
    vector<bool> bits;
    vector<U64> samples;
} RawBits ;

// respresents parsed packet (i.e. cmd or rsp)
typedef struct _ParseResult {
    bool isOK;
    bool dir;
    U64  startbit;
    U64  transmissionbit;
    U64  endbit;
    vector<Frame> frames;
} ParseResult ;

typedef struct _FrameBitBoundaries {
    U8 firstbitidx;
    U8 lastbitidx;
} FrameBitBoundaries;


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
    // stupid state machine:
    PacketType SMgetExpected();
    void               SMinit();
    void               SMputActual(ParseResult);
    // parsing functions:
    U32  ReadBits(U8 nrOfBits);
    bool GetCommandBit();
    RawBits GetBits(U16);
    ParseResult Parse(PacketType, RawBits);

    // create analyzer results 
    void PutResults( ParseResult );

protected: //vars
        PacketType nextExpected;
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
