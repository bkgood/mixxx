// cratetablemodel.cpp
// Created 10/25/2009 by RJ Ryan (rryan@mit.edu)

#include <QtDebug>

#include "library/cratetablemodel.h"
#include "library/queryutil.h"

CrateTableModel::CrateTableModel(QObject* pParent, 
                                 TrackCollection* pTrackCollection,
                                 ConfigObject<ConfigValue>* pConfig,
                                 QList<int> availableDirIds)
        : BaseSqlTableModel(pParent, pTrackCollection,
                            pConfig, availableDirIds,
                            "mixxx.db.model.crate"),
          m_iCrateId(-1),
          m_crateDAO(pTrackCollection->getCrateDAO()) {
}

CrateTableModel::~CrateTableModel() {
}

void CrateTableModel::setTableModel(int crateId,QString name) {
    //qDebug() << "CrateTableModel::setCrate()" << crateId;
    
    if (crateId == m_iCrateId) {
        qDebug() << "Already focused on crate " << crateId;
        return;
    } else if (crateId == -1) {
        // calls from parent class use -1 as id then just set the current crate
        crateId = m_iCrateId;
    }
    
    m_iCrateId = crateId;

    QString tableName = QString("crate_%1").arg(m_iCrateId);
    QSqlQuery query(m_pTrackCollection->getDatabase());
    FieldEscaper escaper(m_pTrackCollection->getDatabase());

    QStringList columns;
    QStringList tableColumns;
    QString filter;
    columns << "crate_tracks."+CRATETRACKSTABLE_TRACKID;
    tableColumns << CRATETRACKSTABLE_TRACKID;
    bool showMissing = m_pConfig->getValueString(ConfigKey("[Library]","ShowMissingSongs"),"1").toInt();
    if(showMissing){
        filter = "library.mixxx_deleted=0";
        tableName.append("_missing");
    } else {
        filter = "library.mixxx_deleted=0 AND track_locations.fs_deleted=0";
    }
    tableName.append(name);
    // We drop files that have been explicitly deleted from mixxx
    // (mixxx_deleted=0) from the view. There was a bug in <= 1.9.0 where
    // removed files were not removed from crates, so some users will have
    // libraries where this is the case.
    QString queryString = QString(
        "CREATE TEMPORARY VIEW IF NOT EXISTS %1 AS "
        "SELECT %2 FROM %3 "
        "INNER JOIN library ON library.id = %3.%4 "
        "INNER JOIN track_locations ON track_locations.id=crate_tracks.track_id "
        "WHERE %3.%5 = %6 AND %7")
            .arg(escaper.escapeString(tableName),
                 columns.join(","),
                 CRATE_TRACKS_TABLE,
                 CRATETRACKSTABLE_TRACKID,
                 CRATETRACKSTABLE_CRATEID,
                 QString::number(crateId),
                 filter);
    QStringList ids;
    foreach(int id, m_availableDirIds){
        ids << QString::number(id);
    }
    queryString.append(" AND track_locations.maindir_id in ("+ids.join(",")+",0)");
    query.prepare(queryString);
    if (!query.exec()) {
        LOG_FAILED_QUERY(query);
    }

    setTable(tableName, tableColumns[0], tableColumns,
             m_pTrackCollection->getTrackSource("default"));
    // BaseSqlTableModel sets up the header names
    initHeaderData();
    setSearch("");
    setDefaultSort(fieldIndex("artist"), Qt::AscendingOrder);
}

int CrateTableModel::addTracks(const QModelIndex& index, QList<QString> locations) {
    Q_UNUSED(index);
    // If a track is dropped but it isn't in the library, then add it because
    // the user probably dropped a file from outside Mixxx into this crate.
    QList<QFileInfo> fileInfoList;
    foreach(QString fileLocation, locations) {
        fileInfoList.append(QFileInfo(fileLocation));
    }

    QList<int> trackIDs = m_trackDAO.addTracks(fileInfoList, true);

    int tracksAdded = m_crateDAO.addTracksToCrate(
        trackIDs, m_iCrateId);
    if (tracksAdded > 0) {
        select();
    }

    if (locations.size() - tracksAdded > 0) {
        qDebug() << "CrateTableModel::addTracks could not add"
                 << locations.size() - tracksAdded
                 << "to crate" << m_iCrateId;
    }
    return tracksAdded;
}

void CrateTableModel::removeTracks(const QModelIndexList& indices) {
    bool locked = m_crateDAO.isCrateLocked(m_iCrateId);

    if (!locked) {
        QList<int> trackIds;
        foreach (QModelIndex index, indices) {
            trackIds.append(getTrackId(index));
        }
        m_crateDAO.removeTracksFromCrate(trackIds, m_iCrateId);
        select();
    }
}

bool CrateTableModel::isColumnInternal(int column) {
    if (column == fieldIndex(CRATETRACKSTABLE_TRACKID) ||
        column == fieldIndex(LIBRARYTABLE_PLAYED) ||
        column == fieldIndex(LIBRARYTABLE_MIXXXDELETED) ||
        column == fieldIndex(LIBRARYTABLE_BPM_LOCK) ||
        column == fieldIndex(TRACKLOCATIONSTABLE_FSDELETED)) {
        return true;
    }
    return false;
}
bool CrateTableModel::isColumnHiddenByDefault(int column) {
    if (column == fieldIndex(LIBRARYTABLE_KEY))
        return true;
    return false;
}

TrackModel::CapabilitiesFlags CrateTableModel::getCapabilities() const {
    CapabilitiesFlags caps =  TRACKMODELCAPS_NONE
            | TRACKMODELCAPS_RECEIVEDROPS
            | TRACKMODELCAPS_ADDTOPLAYLIST
            | TRACKMODELCAPS_ADDTOCRATE
            | TRACKMODELCAPS_ADDTOAUTODJ
            | TRACKMODELCAPS_RELOADMETADATA
            | TRACKMODELCAPS_LOADTODECK
            | TRACKMODELCAPS_LOADTOSAMPLER
            | TRACKMODELCAPS_REMOVE
            | TRACKMODELCAPS_RELOCATE
            | TRACKMODELCAPS_BPMLOCK
            | TRACKMODELCAPS_CLEAR_BEATS
            | TRACKMODELCAPS_RESETPLAYED;

    bool locked = m_crateDAO.isCrateLocked(m_iCrateId);

    if (locked) {
        caps |= TRACKMODELCAPS_LOCKED;
    }

    return caps;
}
