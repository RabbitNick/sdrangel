///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2017 Edouard Griffiths, F4EXB                                   //
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

#include "plutosdrinputsettings.h"

#include <QtGlobal>
#include "util/simpleserializer.h"


PlutoSDRInputSettings::PlutoSDRInputSettings()
{
	resetToDefaults();
}

void PlutoSDRInputSettings::resetToDefaults()
{
	m_centerFrequency = 435000 * 1000;
	m_fcPos = FC_POS_CENTER;
	m_LOppmTenths = 0;
	m_log2Decim = 0;
	m_devSampleRate = 1536 * 1000;
}

QByteArray PlutoSDRInputSettings::serialize() const
{
	SimpleSerializer s(1);

    s.writeS32(1, m_LOppmTenths);
    s.writeU32(4, m_log2Decim);
	s.writeS32(5, m_fcPos);
    s.writeU64(12, m_devSampleRate);

	return s.final();
}

bool PlutoSDRInputSettings::deserialize(const QByteArray& data)
{
	SimpleDeserializer d(data);

	if (!d.isValid())
	{
		resetToDefaults();
		return false;
	}

	if (d.getVersion() == 1)
	{
		int intval;

        d.readS32(1, &m_LOppmTenths, 0);
        d.readU32(4, &m_log2Decim, 0);
		d.readS32(5, &intval, 0);
		m_fcPos = (fcPos_t) intval;
        d.readU64(12, &m_devSampleRate, 1536000U);

		return true;
	}
	else
	{
		resetToDefaults();
		return false;
	}
}