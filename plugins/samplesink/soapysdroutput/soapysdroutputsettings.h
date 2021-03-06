///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2018 Edouard Griffiths, F4EXB                                   //
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

#ifndef PLUGINS_SAMPLESINK_SOAPYSDROUTPUT_SOAPYSDROUTPUTSETTINGS_H_
#define PLUGINS_SAMPLESINK_SOAPYSDROUTPUT_SOAPYSDROUTPUTSETTINGS_H_

#include <QtGlobal>
#include <QMap>

struct SoapySDROutputSettings {
    quint64 m_centerFrequency;
    int m_LOppmTenths;
    qint32 m_devSampleRate;
    quint32 m_log2Interp;
    bool m_transverterMode;
    qint64 m_transverterDeltaFrequency;
    QString m_antenna;
    quint32 m_bandwidth;
    QMap<QString, double> m_tunableElements;
    qint32 m_globalGain;
    QMap<QString, double> m_individualGains;
    bool m_autoGain;
    bool m_autoDCCorrection;
    bool m_autoIQCorrection;
    std::complex<double> m_dcCorrection;
    std::complex<double> m_iqCorrection;

    SoapySDROutputSettings();
    void resetToDefaults();
    QByteArray serialize() const;
    bool deserialize(const QByteArray& data);

private:
    QByteArray serializeNamedElementMap(const QMap<QString, double>& map) const;
    void deserializeNamedElementMap(const QByteArray& data, QMap<QString, double>& map);
};

#endif /* PLUGINS_SAMPLESINK_SOAPYSDROUTPUT_SOAPYSDROUTPUTSETTINGS_H_ */
