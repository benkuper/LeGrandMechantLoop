//
//  ADRS.h
//
//  Created by Nigel Redmon on 12/18/12.
//  EarLevel Engineering: earlevel.com
//  Copyright 2012 Nigel Redmon
//
//  For a complete explanation of the CurvedADSR envelope generator and code,
//  read the series of articles by the author, starting here:
//  http://www.earlevel.com/main/2013/06/01/envelope-generators/
//
//  License:
//
//  This source code is provided as is, without warranty.
//  You may copy and distribute verbatim copies of this document.
//  You may modify and use this source code to create binary code for your own purposes, free or commercial.
//
//  1.01  2016-01-02  njr   added calcCoef to SetTargetRatio functions that were in the CurvedADSR widget but missing in this code
//  1.02  2017-01-04  njr   in calcCoef, checked for rate 0, to support non-IEEE compliant compilers
//  1.03  2020-04-08  njr   changed float to double; large target ratio and rate resulted in exp returning 1 in calcCoef
//

#pragma once

class CurvedADSR {
public:
	CurvedADSR(void);
	~CurvedADSR(void);
	double process(void);
    void applyEnvelopeToBuffer(AudioBuffer<float>& buffer, int startSample, int numSamples);
    double getOutput(void);
    int getState(void);
	void gate(int on);
    void setAttackRate(double rate);
    void setDecayRate(double rate);
    void setReleaseRate(double rate);
	void setSustainLevel(double level);
    void setTargetRatioA(double targetRatio);
    void setTargetRatioDR(double targetRatio);
    void reset(void);

    enum envState {
        env_idle = 0,
        env_attack,
        env_decay,
        env_sustain,
        env_release
    };

protected:
	int state;
	double output;
	double attackRate;
	double decayRate;
	double releaseRate;
	double attackCoef;
	double decayCoef;
	double releaseCoef;
	double sustainLevel;
    double targetRatioA;
    double targetRatioDR;
    double attackBase;
    double decayBase;
    double releaseBase;
 
    double calcCoef(double rate, double targetRatio);
};

inline double CurvedADSR::process() {
	switch (state) {
        case env_idle:
            break;
        case env_attack:
            output = attackBase + output * attackCoef;
            if (output >= 1.0) {
                output = 1.0;
                state = env_decay;
            }
            break;
        case env_decay:
            output = decayBase + output * decayCoef;
            if ((sustainLevel <= 1 && output <= sustainLevel) || (sustainLevel > 1 && output >= sustainLevel)) {
                output = sustainLevel;
                state = env_sustain;
            }
            break;
        case env_sustain:
            break;
        case env_release:
            output = releaseBase + output * releaseCoef;
            if (output <= 0.0) {
                output = 0.0;
                state = env_idle;
            }
	}
	return output;
}

inline void CurvedADSR::gate(int gate) {
	if (gate)
		state = env_attack;
	else if (state != env_idle)
        state = env_release;
}

inline int CurvedADSR::getState() {
    return state;
}

inline void CurvedADSR::reset() {
    state = env_idle;
    output = 0.0;
}

inline double CurvedADSR::getOutput() {
	return output;
}
