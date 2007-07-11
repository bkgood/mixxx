/***************************************************************************
                          dlgbpmtap.h  -  description
                             -------------------
    begin                : Wed Jul 11 2007
    copyright            : (C) 2007 by Micah Lee
    email                : mtl@clemson.edu  
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef DLGBPMTAP_H
#define DLGBPMTAP_H

#include <dlgbpmtapdlg.h>
#include <qevent.h>
#include "configobject.h"

class MixxxApp;
class TrackInfoObject;

/**
  *@author Micah Lee
  */

class DlgBPMTap : public DlgBPMTapDlg
{
    Q_OBJECT
public:
    DlgBPMTap(QWidget *mixxx, TrackInfoObject *tio);
    ~DlgBPMTap();
public slots:
    void slotTapBPM();
    void slotDetectBPM();
    void slotLoadDialog();
    void slotOK();
    void slotUpdate();
    void slotApply();
signals:
    void closeDlg();
    void aboutToShow();
public:
   // void setTrack(Track *track);
protected:
    bool eventFilter(QObject *, QEvent *);
private:
    MixxxApp *m_pMixxx;
    TrackInfoObject *m_CurrentTrack;
    QTime *m_Time;
    int m_TapCount;
};

#endif
