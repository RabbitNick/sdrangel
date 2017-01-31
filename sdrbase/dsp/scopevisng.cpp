///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2017 F4EXB                                                      //
// written by Edouard Griffiths                                                  //
//                                                                               //
// This program is free software; you can redistribute it and/or modify          //
// it under the terms of the GNU General Public License as published by          //
// the Free Software Foundation as version 3 of the License, or                  //
//                                                                               //
// This program is distributed in the hope that it will be useful,               //
// but WITHOUT ANY WARRANTY; without even the implied warranty of                //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                  //
// GNU General Public License V3 for more details.                               //
//                                                                               //
// You should have received a copy of the GNU General Public License             //
// along with this program. If not, see <http://www.gnu.org/licenses/>.          //
///////////////////////////////////////////////////////////////////////////////////

#include <QDebug>
#include "scopevisng.h"
#include "dsp/dspcommands.h"
#include "gui/glscopeng.h"

MESSAGE_CLASS_DEFINITION(ScopeVisNG::MsgConfigureScopeVisNG, Message)
MESSAGE_CLASS_DEFINITION(ScopeVisNG::MsgScopeVisNGAddTrigger, Message)
MESSAGE_CLASS_DEFINITION(ScopeVisNG::MsgScopeVisNGChangeTrigger, Message)
MESSAGE_CLASS_DEFINITION(ScopeVisNG::MsgScopeVisNGRemoveTrigger, Message)
MESSAGE_CLASS_DEFINITION(ScopeVisNG::MsgScopeVisNGAddTrace, Message)
MESSAGE_CLASS_DEFINITION(ScopeVisNG::MsgScopeVisNGChangeTrace, Message)
MESSAGE_CLASS_DEFINITION(ScopeVisNG::MsgScopeVisNGRemoveTrace, Message)

const uint ScopeVisNG::m_traceChunkSize = 4800;
const Real ScopeVisNG::ProjectorMagDB::mult = (10.0f / log2f(10.0f));


ScopeVisNG::ScopeVisNG(GLScopeNG* glScope) :
    m_glScope(glScope),
	m_preTriggerDelay(0),
	m_currentTriggerIndex(0),
	m_triggerState(TriggerUntriggered),
	m_traceSize(m_traceChunkSize),
	m_traceStart(true),
	m_traceFill(0),
	m_zTraceIndex(-1),
	m_traceCompleteCount(0),
	m_timeOfsProMill(0),
	m_sampleRate(0)
{
    setObjectName("ScopeVisNG");
    m_tracebackBuffers.resize(1);
    m_tracebackBuffers[0].resize(4*m_traceChunkSize);
}

ScopeVisNG::~ScopeVisNG()
{
	std::vector<TriggerCondition>::iterator it = m_triggerConditions.begin();

	for (; it != m_triggerConditions.end(); ++it)
	{
		delete it->m_projector;
	}
}

void ScopeVisNG::setSampleRate(int sampleRate)
{
    if (sampleRate != m_sampleRate)
    {
        m_sampleRate = sampleRate;
        if (m_glScope) m_glScope->setSampleRate(m_sampleRate);
    }
}

void ScopeVisNG::configure(uint traceSize)
{
    Message* cmd = MsgConfigureScopeVisNG::create(traceSize);
    getInputMessageQueue()->push(cmd);
}

void ScopeVisNG::addTrace(const TraceData& traceData)
{
    Message* cmd = MsgScopeVisNGAddTrace::create(traceData);
    getInputMessageQueue()->push(cmd);
}

void ScopeVisNG::changeTrace(const TraceData& traceData, uint32_t traceIndex)
{
    Message* cmd = MsgScopeVisNGChangeTrace::create(traceData, traceIndex);
    getInputMessageQueue()->push(cmd);
}

void ScopeVisNG::removeTrace(uint32_t traceIndex)
{
    Message* cmd = MsgScopeVisNGRemoveTrace::create(traceIndex);
    getInputMessageQueue()->push(cmd);
}

void ScopeVisNG::addTrigger(const TriggerData& triggerData)
{
    Message* cmd = MsgScopeVisNGAddTrigger::create(triggerData);
    getInputMessageQueue()->push(cmd);
}

void ScopeVisNG::changeTrigger(const TriggerData& triggerData, uint32_t triggerIndex)
{
    Message* cmd = MsgScopeVisNGChangeTrigger::create(triggerData, triggerIndex);
    getInputMessageQueue()->push(cmd);
}

void ScopeVisNG::removeTrigger(uint32_t triggerIndex)
{
    Message* cmd = MsgScopeVisNGRemoveTrigger::create(triggerIndex);
    getInputMessageQueue()->push(cmd);
}


void ScopeVisNG::feed(const SampleVector::const_iterator& cbegin, const SampleVector::const_iterator& end, bool positiveOnly)
{
	uint32_t feedIndex = 0; // TODO: redefine feed interface so it can be passed a feed index

    if (m_triggerState == TriggerFreeRun) {
        m_triggerPoint = cbegin;
    }
    else if (m_triggerState == TriggerTriggered) {
        m_triggerPoint = cbegin;
    }
    else if (m_triggerState == TriggerUntriggered) {
        m_triggerPoint = end;
    }
    else if (m_triggerState == TriggerWait) {
        m_triggerPoint = end;
    }
    else {
        m_triggerPoint = cbegin;
    }

	if (m_triggerState == TriggerNewConfig)
	{
		m_triggerState = TriggerUntriggered;
		return;
	}

	if ((m_triggerConditions.size() > 0) && (m_triggerState == TriggerWait)) {
		return;
	}

	m_tracebackBuffers[feedIndex].write(cbegin, end);
	SampleVector::const_iterator begin(cbegin);
	TriggerCondition& triggerCondition = m_triggerConditions[m_currentTriggerIndex];

	// trigger process
	if ((m_triggerConditions.size() > 0) && (feedIndex == triggerCondition.m_triggerData.m_inputIndex))
	{
        while (begin < end)
        {
            if (m_triggerState == TriggerUntriggered)
            {
				bool condition = triggerCondition.m_projector->run(*begin) > triggerCondition.m_triggerData.m_triggerLevel;
				bool trigger;

				if (triggerCondition.m_triggerData.m_triggerBothEdges) {
					trigger = triggerCondition.m_prevCondition ^ condition;
				} else {
					trigger = condition ^ !triggerCondition.m_triggerData.m_triggerPositiveEdge;
				}

				if (trigger)
				{
					if (triggerCondition.m_triggerData.m_triggerDelay > 0)
					{
						triggerCondition.m_triggerDelayCount = triggerCondition.m_triggerData.m_triggerDelay;
						m_triggerState == TriggerDelay;
					}
					else
					{
					    if (triggerCondition.m_triggerCounter > 0)
					    {
					        triggerCondition.m_triggerCounter--;
					        m_triggerState = TriggerUntriggered;
					    }
					    else
					    {
					        // next trigger
	                        m_currentTriggerIndex++;

	                        if (m_currentTriggerIndex == m_triggerConditions.size())
	                        {
	                            m_currentTriggerIndex = 0;
	                            m_triggerState = TriggerTriggered;
	                            m_triggerPoint = begin;
	                            triggerCondition.m_triggerCounter = triggerCondition.m_triggerData.m_triggerCounts;
	                            m_traceStart = true;
	                            break;
	                        }
	                        else
	                        {
	                            m_triggerState = TriggerUntriggered;
	                        }
					    }
					}
				}
			}
            else if (m_triggerState == TriggerDelay)
            {
                if (triggerCondition.m_triggerDelayCount > 0)
                {
                    triggerCondition.m_triggerDelayCount--;
                }
                else
                {
                    triggerCondition.m_triggerDelayCount = 0;

                    // next trigger
                    m_currentTriggerIndex++;

                    if (m_currentTriggerIndex == m_triggerConditions.size())
                    {
                        m_currentTriggerIndex = 0;
                        m_triggerState = TriggerTriggered;
                        m_triggerPoint = begin;
                        triggerCondition.m_triggerCounter = triggerCondition.m_triggerData.m_triggerCounts;
                        m_traceStart = true;
                        break;
                    }
                    else
                    {
                        // initialize a new trace
                        m_triggerState = TriggerUntriggered;
                        m_traceCompleteCount = 0;
                        m_triggerState = TriggerUntriggered;

                        feed(begin, end, positiveOnly); // process the rest of samples
                    }
                }
            }
            else
            {
                break;
            }

            ++begin;
		} // begin < end
	}

	// trace process
	if ((m_triggerConditions.size() == 0) || (m_triggerState == TriggerTriggered))
	{
	    // trace back

	    if (m_traceStart)
	    {
	        int count = end - begin; // number of samples in traceback buffer past the current point
	        std::vector<Trace>::iterator itTrace = m_traces.begin();

	        for (;itTrace != m_traces.end(); ++itTrace)
	        {
	            if (itTrace->m_traceData.m_inputIndex == feedIndex)
	            {
	                // TODO: store current point in traceback (current - count)
	                SampleVector::const_iterator startPoint = m_tracebackBuffers[feedIndex].getCurrent() - count;
                    SampleVector::const_iterator prevPoint = m_tracebackBuffers[feedIndex].getCurrent() - count - m_preTriggerDelay - itTrace->m_traceData.m_traceDelay;
                    processPrevTrace(prevPoint, startPoint, itTrace);
	            }
	        }

	        m_traceStart = false;
	    }

	    // live trace

	    int shift = (m_timeOfsProMill / 1000.0) * m_traceSize;

	    while (begin < end)
	    {
	        for (std::vector<Trace>::iterator itTrace = m_traces.begin(); itTrace != m_traces.end(); ++itTrace)
	        {
	            if (itTrace->m_traceData.m_inputIndex == feedIndex)
	            {
	                float posLimit = 1.0 / itTrace->m_traceData.m_amp;
	                float negLimit = -1.0 / itTrace->m_traceData.m_amp;

	                if (itTrace->m_traceCount < m_traceSize)
	                {
	                    float v = itTrace->m_projector->run(*begin) * itTrace->m_traceData.m_amp + itTrace->m_traceData.m_ofs;

	                    if(v > posLimit) {
	                        v = posLimit;
	                    } else if (v < negLimit) {
	                        v = negLimit;
	                    }

	                    itTrace->m_trace[2*(itTrace->m_traceCount)] = itTrace->m_traceCount - shift;
	                    itTrace->m_trace[2*(itTrace->m_traceCount)+1] = v;

	                    itTrace->m_traceCount++;
	                }
	                else
	                {
	                    itTrace->m_traceCount = 0;

	                    if (m_traceCompleteCount < m_traces.size())
	                    {
	                        m_traceCompleteCount++;
	                    }
	                    else
	                    {
	                        // TODO: glScopeNG new traces
	                        // TODO: mark end point in traceback buffer: current - (end - begin)
	                        //m_glScope->newTraces((DisplayTraces&) m_traces);
	                        m_traceCompleteCount = 0;
	                    }
	                }
	            }
	        }

	        begin++;
	    }
	}
}

void ScopeVisNG::processPrevTrace(SampleVector::const_iterator& begin, const SampleVector::const_iterator& end, std::vector<Trace>::iterator& trace)
{
    int shift = (m_timeOfsProMill / 1000.0) * m_traceSize;
    float posLimit = 1.0 / trace->m_traceData.m_amp;
    float negLimit = -1.0 / trace->m_traceData.m_amp;

    while (begin < end)
    {
        float v = trace->m_projector->run(*begin) * trace->m_traceData.m_amp + trace->m_traceData.m_ofs;

        if(v > posLimit) {
            v = posLimit;
        } else if (v < negLimit) {
            v = negLimit;
        }

        trace->m_trace[2*(trace->m_traceCount)] = (trace->m_traceCount - shift); // display x
        trace->m_trace[2*(trace->m_traceCount) + 1] = v;                         // display y

        trace->m_traceCount++;
        begin++;
    }
}

void ScopeVisNG::start()
{
}

void ScopeVisNG::stop()
{
}

bool ScopeVisNG::handleMessage(const Message& message)
{
    qDebug() << "ScopeVisNG::handleMessage" << message.getIdentifier();

    if (DSPSignalNotification::match(message))
    {
        DSPSignalNotification& notif = (DSPSignalNotification&) message;
        setSampleRate(notif.getSampleRate());
        qDebug() << "ScopeVisNG::handleMessage: DSPSignalNotification: m_sampleRate: " << m_sampleRate;
        return true;
    }
    else if (MsgConfigureScopeVisNG::match(message))
    {
        MsgConfigureScopeVisNG& conf = (MsgConfigureScopeVisNG&) message;

    }

}

