#include "CFW21Data.h"
#include <fstream>
#include <vector>
#include "csv_readrow.h"
#include "utctime.h"
#include <iostream>
#include <iomanip>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
using namespace std;
using namespace utctime;


tm CFW21Data::ParseISO8061(const string input)
{
	tm thisTime = { 0 };
	//first, need to know if extended or basic ISO 8061 format, and if Zulu time or time zone offset, also if milliseconds are included(but we'll ignore them...)
	bool isExtended = false;
	bool isZulu = false;
	bool hasMillisecs = false;
	size_t tLoc = input.find('T');
	size_t found = input.find('-');
	if (found != string::npos && found < tLoc)
		isExtended = true;
	found = input.find('Z');
	if (found != string::npos)
		isZulu = true;
	found = input.find('.');
	if (found != string::npos)
		hasMillisecs = true;
	//then do appropriate sscanf!
	int nRead;
	int y = -1, M = -1, d = -1, h = -1, m = -1, s = -1, tzh = 0, tzm = 0;
	float ms = 0.0;
	if (isExtended)//has dashes separating fields...
	{
		if (isZulu)
		{
			if(hasMillisecs)
				nRead = sscanf(input.c_str(), "%d-%d-%dT%d:%d:%fZ", &y, &M, &d, &h, &m, &ms);
			else
				nRead = sscanf(input.c_str(), "%d-%d-%dT%d:%d:%dZ", &y, &M, &d, &h, &m, &s);
		}
		else
		{
			if (hasMillisecs)
				nRead = sscanf(input.c_str(), "%d-%d-%dT%d:%d:%f%d%d", &y, &M, &d, &h, &m, &ms, &tzh, &tzm);
			else
				nRead = sscanf(input.c_str(), "%d-%d-%dT%d:%d:%d%3d:%d", &y, &M, &d, &h, &m, &s, &tzh, &tzm);
		}
	}
	else//basic
	{
		if (isZulu)
		{
			if (hasMillisecs)
				nRead = sscanf(input.c_str(), "%4d%2d%2dT%2d%2d%fZ", &y, &M, &d, &h, &m, &ms);
			else
				nRead = sscanf(input.c_str(), "%4d%2d%2dT%2d%2d%2dZ", &y, &M, &d, &h, &m, &s);
		}
		else
		{
			if (hasMillisecs)
				nRead = sscanf(input.c_str(), "%4d%2d%2dT%2d%2d%f%3d%2d", &y, &M, &d, &h, &m, &ms, &tzh, &tzm);
			else
				nRead = sscanf(input.c_str(), "%4d%2d%2dT%2d%2d%2d%3d%2d", &y, &M, &d, &h, &m, &s, &tzh, &tzm);
		}
	}
	//now build the tm
	if (isZulu)//convert to local time
	{
		UTCTime zTime(y, M, d, h, m, s);
		thisTime = zTime.get_tm();
		tm_increment_hour(&thisTime, m_timeZoneOffset);
	}
	else
	{
		UTCTime lTime(y, M, d, h, m, s);
		thisTime = lTime.get_tm();
	}
	return thisTime;
}

FW21Record::FW21Record()
{
	memset(&m_dateTime, 0, sizeof(m_dateTime));
	m_temp = dNODATA;
	m_RH = dNODATA;
	m_pcp = dNODATA;
	m_windSpeed = dNODATA;
	m_windAzimuth = iNODATA;
	m_solarRadiation = dNODATA;
	m_snowFlag = iNODATA;
	m_gustSpeed = dNODATA;
	m_gustAzimuth = iNODATA;
}

FW21Record::FW21Record(const FW21Record& rhs)
{
	m_dateTime = rhs.m_dateTime;
	m_temp = rhs.m_temp;
	m_RH = rhs.m_RH;
	m_pcp = rhs.m_pcp;
	m_windSpeed = rhs.m_windSpeed;
	m_windAzimuth = rhs.m_windAzimuth;
	m_solarRadiation = rhs.m_solarRadiation;
	m_snowFlag = rhs.m_snowFlag;
	m_gustSpeed = rhs.m_gustSpeed;
	m_gustAzimuth = rhs.m_gustAzimuth;
}
FW21Record::~FW21Record()
{

}

NFDRSRec::NFDRSRec() : FW21Record()
{
	m_minTemp = dNODATA;
	m_maxTemp = dNODATA;
	m_minRH = dNODATA;
	m_pcp24 = dNODATA;
}

NFDRSRec::NFDRSRec(const NFDRSRec& rhs) : FW21Record(rhs)
{
	m_minTemp = rhs.m_minTemp;
	m_maxTemp = rhs.m_maxTemp;
	m_minRH = rhs.m_minRH;
	m_pcp24 = rhs.m_pcp24;
}

NFDRSRec::NFDRSRec(FW21Record rhs)
{
	SetDateTime(rhs.GetDateTime());
	SetTemp(rhs.GetTemp());
	SetRH(rhs.GetRH());
	SetPrecip(rhs.GetPrecip());
	SetWindSpeed(rhs.GetWindSpeed());
	SetSolarRadiation(rhs.GetSolarRadiation());
	SetSnowFlag(rhs.GetSnowFlag());
}

NFDRSRec::~NFDRSRec()
{

}

CFW21Data::CFW21Data()
{
	m_fileName = "";
	m_bTimeIsZulu = false;
	m_timeZoneOffset = 0;
}

CFW21Data::CFW21Data(const CFW21Data& rhs)
{//not implemented!!!!

}

CFW21Data::~CFW21Data()
{

}


int CFW21Data::LoadFile(const char *fw21FileName, int tzOffsetHours/* = 0*/)
{
	m_timeZoneOffset = tzOffsetHours;
	//vector<string> vFieldNames = { "DateTime","Temperature(F)","RelativeHumidity(%)","Precipitation(in)",
	//	"WindSpeed(mph)","WindAzimuth(degrees)","SolarRadiation(W/m2)","SnowFlag","GustSpeed(mph)","GustAzimuth(degrees)" };
	m_fileName = fw21FileName;
	ifstream stream;
	stream.open(m_fileName);
	if (!stream.is_open())
	{
		printf("Error opening %s as input\n", m_fileName.c_str());
		return -1;
	}
	char buf[1024];
	//get the header line which contains FW12 fields
	int bufSize = 1024;
	stream.getline(buf, bufSize);
	string line = buf;
	vector<string> vFields = csv_read_row(line, ',');
	//get field Indexes
	int dtIdx, tmpIdx, rhIdx, pcpIdx, wsIdx, wdirIdx, srIdx, snowIdx, gsIdx, gdirIdx;
	dtIdx = getColIndex(m_vFieldNames[0], vFields);
	tmpIdx = getColIndex(m_vFieldNames[1], vFields);
	rhIdx = getColIndex(m_vFieldNames[2], vFields);
	pcpIdx = getColIndex(m_vFieldNames[3], vFields);
	wsIdx = getColIndex(m_vFieldNames[4], vFields);
	wdirIdx = getColIndex(m_vFieldNames[5], vFields);
	srIdx = getColIndex(m_vFieldNames[6], vFields);
	snowIdx = getColIndex(m_vFieldNames[7], vFields);
	gsIdx = getColIndex(m_vFieldNames[8], vFields);
	gdirIdx = getColIndex(m_vFieldNames[9], vFields);

	string strDate, strTemp, strRH, strPcp, strWindSpeed, strWDir, strSolRad, strSnow, strGustSpeed, strGustDir;
	//basic check for required fields
	if (dtIdx < 0 || tmpIdx < 0 || rhIdx < 0 || pcpIdx < 0 || wsIdx < 0 || wdirIdx < 0 || srIdx < 0 || snowIdx < 0)
	{
		if (dtIdx < 0)
			printf("Error, field %s not found in header\n", m_vFieldNames[0].c_str());
		if (tmpIdx < 0)
			printf("Error, field %s not found in header\n", m_vFieldNames[1].c_str());
		if (rhIdx < 0)
			printf("Error, field %s not found in header\n", m_vFieldNames[2].c_str());
		if (pcpIdx < 0)
			printf("Error, field %s not found in header\n", m_vFieldNames[3].c_str());
		if (wsIdx < 0)
			printf("Error, field %s not found in header\n", m_vFieldNames[4].c_str());
		if (wdirIdx < 0)
			printf("Error, field %s not found in header\n", m_vFieldNames[5].c_str());
		if (srIdx < 0)
			printf("Error, field %s not found in header\n", m_vFieldNames[6].c_str());
		if (snowIdx < 0)
			printf("Error, field %s not found in header\n", m_vFieldNames[7].c_str());
		printf("Header line is:\n%s\n", buf);
		stream.close();
		return -2;
	}
	//ok, ready to parse the data...
	bool firstRec = false;
	int lineNo = 2;
	while (stream.good())
	{
		stream.getline(buf, bufSize);
		lineNo++;
		line = buf;
		vFields = csv_read_row(line, ',');
		if (vFields.size() < 8)
		{
			printf("Warning, line %d has less than 8 fields, skipping record\n", lineNo);
			continue;
		}
		FW21Record thisRec;
		strDate = vFields[dtIdx];
		trim(strDate);
		if (strDate.empty())
		{
			printf("Error: DateTime is blank, line %d\n", lineNo);
			continue;
			continue;
		}
		if (firstRec)
		{
			//need to check for Zulu time
			if (strDate.find('Z') != string::npos)
				m_bTimeIsZulu = true;
			firstRec = false;
		}
		tm recTime = ParseISO8061(strDate);
		thisRec.SetDateTime(recTime);
		strTemp = vFields[tmpIdx];
		trim(strTemp);
		if (!strTemp.empty())
			thisRec.SetTemp(atof(strTemp.c_str()));
		strRH = vFields[rhIdx];
		trim(strRH);
		if (!strRH.empty())
			thisRec.SetRH(max(atof(strRH.c_str()), 1.0));
		strPcp = vFields[pcpIdx];
		trim(strPcp);
		if (!strPcp.empty())
			thisRec.SetPrecip(atof(strPcp.c_str()));
		strWindSpeed = vFields[wsIdx];
		trim(strWindSpeed);
		if (!strWindSpeed.empty())
			thisRec.SetWindSpeed(atof(strWindSpeed.c_str()));
		strWDir = vFields[wdirIdx];
		trim(strWDir);
		if(!strWDir.empty())
			thisRec.SetWindAzimuth(atoi(strWDir.c_str()));
		strSolRad = vFields[srIdx];
		trim(strSolRad);
		if (!strSolRad.empty())
			thisRec.SetSolarRadiation(atof(strSolRad.c_str()));
		strSnow = vFields[snowIdx];
		trim(strSnow);
		if (!strSnow.empty())
			thisRec.SetSnowFlag(atoi(strSnow.c_str()));
		else // assume not snow covered
			thisRec.SetSnowFlag(0);
		if (gsIdx >= 0)
		{
			strGustSpeed = vFields[gsIdx];
			trim(strGustSpeed);
			if (!strGustSpeed.empty())
				thisRec.SetGustSpeed(atof(strGustSpeed.c_str()));
		}
		if (gdirIdx >= 0)
		{
			strGustDir = vFields[gdirIdx];
			trim(strGustDir);
			if (!strGustDir.empty())
				thisRec.SetGustAzimuth(atoi(strGustDir.c_str()));
		}
		bool goodRec = true;
		//first, check for blanks on key fields
		if (strDate.length() <= 0)
		{
			printf("Error: DateTime is blank, line %d\n", lineNo);
			continue;
		}
		if (strTemp.length() <= 0)
		{
			printf("Error: Temperature(F) is blank, line %d, DateTime: %s\n", lineNo, strDate.c_str());
			continue;
		}
		if (strRH.length() <= 0)
		{
			printf("Error: RelativeHumidity(%%) is blank, line %d, DateTime: %s\n", lineNo, strDate.c_str());
			continue;
		}
		if (strPcp.length() <= 0)
		{
			printf("Error: Precipitation(in) is blank, line %d, DateTime: %s\n", lineNo, strDate.c_str());
			continue;
		}
		if (strSolRad.length() <= 0)
		{
			printf("Error: SolarRadiation(W/m2) is blank, line %d, DateTime: %s\n", lineNo, strDate.c_str());
			continue;
		}
		//now some range checks
		if (thisRec.GetTemp() < -76.0 || thisRec.GetTemp() > 140.0)
		{
			printf("Error: Bad Temperature(F) line %d, %.1f, DateTime: %s\n", lineNo, thisRec.GetTemp(), strDate.c_str());
			continue;
		}
		if (thisRec.GetRH() <= 0.0 || thisRec.GetRH() > 100.0)
		{
			printf("Error: Bad RelativeHumidity(%%) line %d, %.1f, DateTime: %s\n", lineNo, thisRec.GetRH(), strDate.c_str());
			continue;
		}
		if (thisRec.GetPrecip() < 0.0 || thisRec.GetPrecip() > 20.0)
		{
			printf("Error: Bad Precipitation(in) line %d, %.1f, DateTime: %s\n", lineNo, thisRec.GetPrecip(), strDate.c_str());
			continue;
		}
		if (thisRec.GetSolarRadiation() < 0.0 || thisRec.GetSolarRadiation() > 2000.0)
		{
			printf("Error: Bad SolarRadiation(W/m2) line %d, %.1f, DateTime: %s\n", lineNo, thisRec.GetSolarRadiation(), strDate.c_str());
			continue;
		}
		//non-fatal warnings
		if (thisRec.GetWindSpeed() < 0.0 || thisRec.GetWindSpeed() > 99.0)
		{
			printf("Warning: Bad WindSpeed(mph) line %d, %.1f, DateTime: %s\n", lineNo, thisRec.GetWindSpeed(), strDate.c_str());
		}
		if (thisRec.GetWindAzimuth() < 0 || thisRec.GetWindAzimuth() > 360)
		{
			printf("Warning: Bad WindAzimuth(degrees) line %d, %d, DateTime: %s\n", lineNo, thisRec.GetWindAzimuth(), strDate.c_str());
		}

		//if we got here record is acceptable
		m_recs.push_back(thisRec);
	}
	stream.close();
	return 0;
}

FW21Record CFW21Data::GetRec(size_t recNum)//zero based! valid: 0->GetNumRecs() - 1
{
	FW21Record ret;
	if (recNum >= 0 && recNum < m_recs.size())
		ret = m_recs[recNum];
	return ret;
}

const double SECS_PER_DAY = 86400.0;

NFDRSRec CFW21Data::GetNFDRSRec(size_t recNum)//zero based! valid: 0->GetNumRecs() - 1
{
	FW21Record rec, rec2;
	if (recNum < 0 || recNum >= m_recs.size())
	{
		NFDRSRec ret;
		return ret;
	}
	rec = GetRec(recNum);
	NFDRSRec goodRec(rec);
	tm trgTime = rec.GetDateTime();
	time_t trgTimet = mktime(&trgTime), thisTimet;
	double tMin = rec.GetTemp(), tMax = rec.GetTemp(), rhMin = rec.GetRH(), pcp = rec.GetPrecip();
	long checkRec = recNum - 1;
	while (checkRec >= 0)
	{
		rec2 = GetRec(checkRec);
		tm thisTime = rec2.GetDateTime();
		thisTimet = mktime(&thisTime);
		if (difftime(trgTimet, thisTimet) >= SECS_PER_DAY)
			break;
		double thisTemp = rec2.GetTemp(), thisRH = rec2.GetRH(), thisPcp = rec2.GetPrecip();
		if (thisTemp != dNODATA)
		{
			if (thisTemp < tMin || tMin == dNODATA)
				tMin = thisTemp;
			if (thisTemp > tMax || tMax == dNODATA)
				tMax = thisTemp;
		}
		if (thisRH != dNODATA)
		{
			if (thisRH < rhMin || rhMin == dNODATA)
				rhMin = thisRH;
		}
		if (thisPcp != dNODATA || pcp == dNODATA)
			pcp += thisPcp;
		checkRec--;
	}
	goodRec.SetMinTemp(tMin);
	goodRec.SetMaxTemp(tMax);
	goodRec.SetMinRH(rhMin);
	goodRec.SetPcp24(pcp);
	return goodRec;
}

int CFW21Data::AddRecord(FW21Record rec)
{
	if (m_recs.size() > 0)//ensure rec is after last record
	{
		FW21Record lastRec = m_recs[m_recs.size() - 1];
		UTCTime lastUtc(lastRec.GetYear(), lastRec.GetMonth(), lastRec.GetDay(), lastRec.GetHour(), lastRec.GetMinutes(), 0);
		UTCTime recUtc(rec.GetYear(), rec.GetMonth(), rec.GetDay(), rec.GetHour(), rec.GetMinutes(), 0);
		//tm recTM = rec.GetDateTime();
		//time_t recTime = mktime(&recTM);
		//tm lastTM = m_recs[m_recs.size() - 1].GetDateTime();
		//time_t lastTime = mktime(&lastTM);
		//if (recTime - lastTime <= 0)
		if (recUtc <= lastUtc)
		{
			cout << "Error, rectime is <= last record time\n";
			return -1;
		}
	}
	m_recs.push_back(rec);
	return 1;
}

string FormatTM(tm in, int offsetHours)
{
	char buf[64];
	string ret = "";
	if(offsetHours < 0)
		sprintf(buf, "%4d-%02d-%02dT%02d:%02d:00%03d:00",
			in.tm_year + 1900, in.tm_mon + 1, in.tm_mday,
			in.tm_hour, in.tm_min, offsetHours);
	else
		sprintf(buf, "%4d-%02d-%02dT%02d:%02d:00%02d:00",
			in.tm_year + 1900, in.tm_mon + 1, in.tm_mday,
			in.tm_hour, in.tm_min, offsetHours);

	ret = buf;
	return ret;
}

int CFW21Data::WriteFile(const char* fw21FileName, int offsetHours)
{
	ofstream out;
	out.open(fw21FileName, ofstream::out | ofstream::trunc);
	for (vector<string>::iterator it = m_vFieldNames.begin(); it != m_vFieldNames.end(); ++it)
	{
		if (it != m_vFieldNames.begin())
			out << ", ";
		out << *it;
	}
	out << "\n";


	for (vector<FW21Record>::iterator it = m_recs.begin(); it != m_recs.end(); ++it)
	{
		FW21Record rec = *it;
		string dateStr = FormatTM(rec.GetDateTime(), offsetHours);
		out << dateStr << ",";
		
		out << std::fixed << std::setw(1) << std::setprecision(0) << rec.GetTemp() << ",";
		out << std::fixed << std::setw(1) << std::setprecision(0) << rec.GetRH() << ",";
		out << std::fixed << std::setw(5) << std::setprecision(3) << rec.GetPrecip() << ",";
		out << std::fixed << std::setw(1) << std::setprecision(0) << rec.GetWindSpeed() << ",";
		out << rec.GetWindAzimuth() << ",";
		out << std::fixed << std::setw(1) << std::setprecision(0) << rec.GetSolarRadiation() << ",";
		out << rec.GetSnowFlag() << ",";
		out << std::fixed << std::setw(1) << std::setprecision(0) << rec.GetGustSpeed() << ",";
		out << rec.GetGustAzimuth() << "\n";
	}

	out.close();
	return 1;
}
