#ifndef CMARKER_H
#define CMARKER_H

#define MAX_MARKER_SETS 8

#define MARKER_NOT_MARKED 0xFFFFFFFF

typedef enum
{
   eMarkerSet_Invalid = 0,
   eMarkerSet_Started,
   eMarkerSet_Complete
} eMarkerSet_State;

typedef struct _MarkerSetInfo
{
   eMarkerSet_State state;
   unsigned int     startAbsAddr;
   unsigned int     endAbsAddr;
   unsigned int     startFrame;
   unsigned int     startCycle;
   unsigned int     endFrame;
   unsigned int     endCycle;
   unsigned char    red;
   unsigned char    green;
   unsigned char    blue;
} MarkerSetInfo;

class CMarker
{
public:
   CMarker();
   int GetNumMarkers(void) const { return MAX_MARKER_SETS; }
   MarkerSetInfo* GetMarker(int marker) { return m_marker+marker; }
   int AddMarker(unsigned int absAddr);
   int FindInProgressMarker(void);
   void ClearAllMarkers(void);
   void ZeroAllMarkers(void);
   void CompleteMarker(int marker, unsigned int absAddr);
   void UpdateMarkers(unsigned int absAddr, unsigned int frame, unsigned int cycle);

protected:
   MarkerSetInfo m_marker [ MAX_MARKER_SETS ];
};

#endif // CMARKER_H
