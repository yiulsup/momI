#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

static float convert(short);
static float after_convert(float);
static short search_max(void);
static float calibration(float);
static int RaiderData(char*);

static float temp[80][60];
static float bodytemp[80][60];
static float blackbodytemp[80][60];
// store raw data.
static unsigned short gs_raw_data[80][60];
static unsigned short gs_raw_data2[4800];

static float m_fCalibration = 0.0;
static float m_fOffsetDelta = 2.0;
static float m_fBlackbodyDelta = 0.0;
static float m_fCorectDistanceDelta = 0.0;
static float m_fAutoAlarmTempValue = 0.0;

static int gs_calibration_flag = 0;
static int m_bPushAlarmOnOff = 1;

short m_fBlackBodyMaxVal = 0;
float m_fBlackBodyDegree = 0;
float m_fBodyMaxDegree = 0;
float m_fBodyMinDegree = 0;

short m_sMax = (short)0;
short m_sMin = (short)4000;

float m_fBlackBodyMax = (float)0;
float m_fBodyMaxVal = (float)0;
float m_fBodyMinVal = (float)4000;
float m_fBlackBodyMin = (float)4000;

float m_fBodyMax = (float)0;
float m_fBodyMin = (float)4000;

int m_nCorectCount = 0;

typedef struct coordi {
	int x;
	int y;
} t_coordi;

static t_coordi blackbody_max(void);
static t_coordi body_max(void);
static t_coordi blackbody_cam_xy;
static t_coordi body_cam_xy;

extern float SK_P2P_GetCalibTemp_2(void);

int SK_IRCAM_init(void)
{
	return 0;
}

float SK_IRCAM_setCalibrationTemp(float fCalibrationTemp)
{
	float fMaxVal = search_max();

	m_fCalibration = fCalibrationTemp - (fMaxVal - m_fBlackbodyDelta - m_fCalibration);

	return m_fCalibration;
}

union Uint16With2Byte {
	unsigned short Data;
	unsigned char  byte[2];
} mDataConvertion;

#define  MAX_RECEIVE			(20000 + 1) // (8192 + 1)
unsigned char RecvData[MAX_RECEIVE];

short SK_IRCAM_getBodyTemp(unsigned char* raw_data, int len) //, char* pRaiderData)
{
	//char* a_string = malloc(128);
	unsigned short raw_data2[9633] = { 0, };
	int nC = 0;
	unsigned char  nIData[50] = { 0, };
	unsigned char ccci[1] = { 0, };
	//FILE* fp = fopen("test.txt","a");
	FILE* fp = fopen("raw1.txt","rt+");
	printf("start \n");
	short raw_data2_Header[50] = { 0, };
	int nCHeader = 0;
	for (int n = 0; len > n; n++) {
		//printf(" %hd", raw_data[n]);
		if (n >= 31) {
			if ((n % 2) == 1) {
				mDataConvertion.byte[1] = raw_data[n];
			}
			else {
				mDataConvertion.byte[0] = raw_data[n];
				raw_data2[nC] = mDataConvertion.Data;
				nC++;
			}
		}
		else {
			raw_data2_Header[nCHeader] = raw_data[n];
			//printf("%hu ", raw_data2_Header[nCHeader]);
			nCHeader;
		}
	}
	printf("end \n");

	memcpy(gs_raw_data, raw_data2, nC * sizeof(unsigned short));
	memcpy(gs_raw_data2, raw_data2, nC * sizeof(unsigned short));


	for (int j = 0; j < 4800; j++)
	{

		if (j > 4784)
			gs_raw_data2[j] = gs_raw_data2[4784];
		//printf("%hu ", gs_raw_data2[j]);
                fprintf(fp, "%d ", gs_raw_data2[j]);
	}

	fclose(fp);

	int k = 0;
	for (int j = 0; j < 60; j++)
	{
		for (int i = 0; i < 80; i++)
		{
			temp[i][j] = gs_raw_data2[j * 80 + i];
		}
	}


	m_fBlackBodyMax = 0;
	m_fBlackBodyMin = 4000;
	for (int j = 0; j < 60; j++)
	{
		for (int i = 0; i < 80; i++)
		{
			blackbodytemp[i][j] = temp[i][j];
			//printf("%.0f ", blackbodytemp[i][j]);			

			if (convert(blackbodytemp[i][j]) >= m_fBlackBodyMax)
			{
				if (blackbodytemp[i][j] < 3000) {
					m_fBlackBodyMax = convert(blackbodytemp[i][j]);
					//printf("BlackBodyMax %.0f ", m_fBlackBodyMax);
				}
			}

			if (convert(blackbodytemp[i][j]) <= m_fBlackBodyMin)
			{
				if (blackbodytemp[i][j] > 1000) {
					m_fBlackBodyMin = convert(blackbodytemp[i][j]);
					//printf("BlackBodyMin %.0f ", m_fBlackBodyMin);
				}
			}
		}
	}

	//printf("\nBlackBodyMax %.0f BlackBodyMin %.0f ", m_fBlackBodyMax, m_fBlackBodyMin);


	m_fBodyMax = 0;
	m_fBodyMin = 4000;

	for (int j = 0; j < 60; j++)
	{
		for (int i = 0; i < 80; i++) {

			bodytemp[i][j] = temp[i][j];

			if (convert(bodytemp[i][j]) >= m_fBodyMax)
			{
				if (bodytemp[i][j] < 2500) {
					m_fBodyMax = convert(bodytemp[i][j]);
					//printf("BlackBodyMax %.0f ", m_fBlackBodyMax);
				}
			}

			if (convert(bodytemp[i][j]) <= m_fBodyMin)
			{
				if (bodytemp[i][j] > 1000) {
					m_fBodyMin = convert(bodytemp[i][j]);
					//printf("BlackBodyMin %.0f ", m_fBlackBodyMin);
				}
			}
		}
	}

	/*
	m_fBlackBodyMaxVal = 0;
	blackbody_cam_xy = blackbody_max();
	float bodytemp_max = search_max();

	m_fBodyMaxVal = 0;
	m_fBodyMinVal = 4000;
	body_cam_xy = body_max();

	bodytemp_max = convert(m_fBodyMaxVal) + m_fCorectDistanceDelta;

	//printf("2. %.2f, %.2f, %.2f, %.2f \n", m_fBodyMaxVal * m_fBodyMaxDegree - 6, m_fBodyMinVal * m_fBodyMinDegree - 6, m_fBlackBodyMaxVal * m_fBlackBodyDegree, m_fCalibration);

	return bodytemp_max;	*/
	return 1;
}

static float convert(short nPixelValue)
{
	return (nPixelValue * 0.0636) - 79.9;
}

static float calibration(float fCalibrationTemp)
{
	float fMaxVal = search_max();

	m_fCalibration = fCalibrationTemp - (fMaxVal - m_fBlackbodyDelta - m_fCalibration);

	m_fAutoAlarmTempValue = fMaxVal + m_fBlackbodyDelta + m_fCalibration + m_fOffsetDelta;

	return m_fCalibration;
}

static float offset(int bSymbol, float fValTemp)
{
	if (bSymbol == 0)
	{
		m_fOffsetDelta = -fValTemp;
	}
	else {
		m_fOffsetDelta = fValTemp;
	}
	return m_fOffsetDelta;
}

static short search_max(void)
{
	short fMaxVal = 0;
	printf("start ! \n");
	for (int j = 0; j < 60; j++)
		for (int i = 21; i < 80; i++)
		{
			if (bodytemp[i][j] > fMaxVal) {
				fMaxVal = bodytemp[i][j];
			}
			//printf("%.0f ", bodytemp[i][j]);
		}

	return fMaxVal;
}


static t_coordi blackbody_max(void)
{
	int x = 0;
	int y = 0;
	t_coordi tmp;

	tmp.x = 0;
	tmp.y = 0;

	for (int j = 0; j < 60; j++)
	{
		for (int i = 0; i < 20; i++)
		{
			if (blackbodytemp[i][j] > m_fBlackBodyMaxVal) {
				m_fBlackBodyMaxVal = blackbodytemp[i][j];
				tmp.x = i;
				tmp.y = j;

				//m_fBlackbodyDelta = 40.0 - convert(m_fBlackBodyMaxVal);
				float fBlackBodyMax = (float)m_fBlackBodyMaxVal;
				m_fBlackBodyDegree = fBlackBodyMax / 60.0;
				m_fBlackBodyDegree = 1 / m_fBlackBodyDegree;
				m_fBlackbodyDelta = m_fBlackBodyMaxVal * m_fBlackBodyDegree;
				//printf("blackbody_max %u\n", m_fBlackBodyMaxVal);
			}
			//printf(" %.0f", blackbodytemp[i][j]);
		}
	}

	printf("\nblackbody_max, x:%d, y:%d, %.5f, %.5f, %u\n", tmp.x, tmp.y, m_fBlackBodyDegree, m_fBlackbodyDelta, m_fBlackBodyMaxVal);
	return tmp;
}



static t_coordi body_max(void)
{
	int x = 0;
	int y = 0;
	t_coordi max_tmp;

	max_tmp.x = 0;
	max_tmp.y = 0;

	t_coordi min_tmp;

	min_tmp.x = 0;
	min_tmp.y = 0;
	float fBodyMax = 0;
	float fBodyMin = 200;
	float fValue, fMinValue;

	for (int j = 0; j < 60; j++)
	{
		for (int i = 21; i < 80; i++)
		{
			if (bodytemp[i][j] > m_fBodyMaxVal) {
				m_fBodyMaxVal = bodytemp[i][j];
				max_tmp.x = i;
				max_tmp.y = j;
				float fBlackBodyMax = (float)m_fBlackBodyMaxVal;
				fValue = fBlackBodyMax - m_fBodyMaxVal;
				fValue = fValue * m_fBlackBodyDegree;

				fBodyMax = m_fBodyMaxVal * m_fBlackBodyDegree;
				fBodyMax = fBodyMax - fValue;
				if (fBodyMax < 50.0 && fBodyMax >= 45.0)
				{
					float fDegree = 0.0302 - 0.0285;
					float fBody = 50 - fBodyMax;

					fBody = fBody / 5;
					fDegree = fBody * fDegree;

					m_fBodyMaxDegree = 0.0302 - fDegree;
				}
				else if (fBodyMax < 45.0 && fBodyMax >= 40.0)
				{
					float fDegree = 0.0285 - 0.0242;
					float fBody = 45 - fBodyMax;

					fBody = fBody / 5;
					fDegree = fBody * fDegree;

					m_fBodyMaxDegree = 0.0285 - fDegree;
				}
				else if (fBodyMax < 40.0 && fBodyMax >= 35.0)
				{
					float fDegree = 0.0242 - 0.0217;
					float fBody = 40 - fBodyMax;

					fBody = fBody / 5;
					fDegree = fBody * fDegree;

					m_fBodyMaxDegree = 0.0242 - fDegree;
				}
				else if (fBodyMax < 35.0 && fBodyMax >= 30.0)
				{
					float fDegree = 0.0217 - 0.019;
					float fBody = 35 - fBodyMax;

					fBody = fBody / 5;
					fDegree = fBody * fDegree;

					m_fBodyMaxDegree = 0.0217 - fDegree;
				}
				else if (fBodyMax < 30.0)
				{
					m_fBodyMaxDegree = 0.019;
				}
			}

			if (bodytemp[i][j] < m_fBodyMinVal) {
				m_fBodyMinVal = bodytemp[i][j];
				min_tmp.x = i;
				min_tmp.y = j;


				float fBlackBodyMax = (float)m_fBlackBodyMaxVal;
				fMinValue = fBlackBodyMax - m_fBodyMinVal;
				fMinValue = fMinValue * m_fBlackBodyDegree;

				fBodyMin = m_fBodyMinVal * m_fBlackBodyDegree;
				fBodyMin = fBodyMin - fMinValue;
				if (fBodyMin < 50.0 && fBodyMin >= 45.0)
				{
					float fDegree = 0.0302 - 0.0285;
					float fBody = 50 - fBodyMin;

					fBody = fBody / 5;
					fDegree = fBody * fDegree;

					m_fBodyMinDegree = 0.0302 - fDegree;
				}
				else if (fBodyMin < 45.0 && fBodyMin >= 40.0)
				{
					float fDegree = 0.0285 - 0.0242;
					float fBody = 45 - fBodyMin;

					fBody = fBody / 5;
					fDegree = fBody * fDegree;

					m_fBodyMinDegree = 0.0285 - fDegree;
				}
				else if (fBodyMin < 40.0 && fBodyMin >= 35.0)
				{
					float fDegree = 0.0242 - 0.0217;
					float fBody = 40 - fBodyMin;

					fBody = fBody / 5;
					fDegree = fBody * fDegree;

					m_fBodyMinDegree = 0.0242 - fDegree;
				}
				else if (fBodyMin < 35.0 && fBodyMin >= 30.0)
				{
					float fDegree = 0.0217 - 0.019;
					float fBody = 35 - fBodyMin;

					fBody = fBody / 5;
					fDegree = fBody * fDegree;

					m_fBodyMinDegree = 0.0217 - fDegree;
				}
				else if (fBodyMin < 30.0)
				{
					m_fBodyMinDegree = 0.019;
				}

			}
		}
	}

	//printf("body_max, x:%d, y:%d, %u, %.2f, %.2f, MaxVal = %.2f, %.2f, %.5f\n", max_tmp.x, max_tmp.y, m_fBlackBodyMaxVal, fValue, m_fBodyMaxVal, m_fBodyMaxVal * m_fBodyMaxDegree - 6, fBodyMax, m_fBodyMaxDegree);
	//printf("body_min, x:%d, y:%d, %u, %.2f, %.2f, MinVal = %.2f, %.2f, %.5f\n", min_tmp.x, min_tmp.y, m_fBlackBodyMaxVal, fMinValue, m_fBodyMinVal, m_fBodyMinVal * m_fBodyMinDegree - 6, fBodyMin, m_fBodyMinDegree);
	return max_tmp;
}

static int RaiderData(char* pRaiderData)
{

	char* pData = pRaiderData;
	char const* Data = "no One";
        printf("###################################\n");
	printf("radar sensor : %s \n", pRaiderData);
	if (strstr(pData, Data) != NULL)
	{
		m_fCalibration = 0.0;
		m_bPushAlarmOnOff = 0;
		return 0;
	}

	Data = "moving";
	if (strstr(pData, Data) != NULL)
	{
		//return 0;
	}

	if (m_fCalibration == 0.0)
	{
		calibration(30);
	}

	m_bPushAlarmOnOff = 1;

	char* ptr = strstr(pData, "Distance");

	char szDistance[5] = " ";
	if (ptr != NULL) {
		ptr = strchr(ptr, ':');
		ptr = strchr(ptr, ' ');
	}
	else {
		return 0;
	}
	char szChar;
	int n = 0, nf = 0;;
	while (ptr != NULL)
	{
		if (ptr == NULL) {
			n = n + 1;
			continue;
		}
		else if (ptr != NULL) {
			szChar = ptr[nf];
			if (szChar == 'm') {
				break;
			}
			else if (szChar == ' ') {
				nf = nf + 1;
				continue;
			}
			else {
				szDistance[n] = szChar;
			}
			nf = nf + 1;
			n = n + 1;
		}
		if (n == 5)
			break;
	}

	float fDistance = atof(szDistance);
	if (fDistance > 0.0) {
		m_fCorectDistanceDelta = fDistance;
	}


	if (m_fAutoAlarmTempValue >= search_max())
	{
		//Alarm App Push function
	}


	return 1;
}
