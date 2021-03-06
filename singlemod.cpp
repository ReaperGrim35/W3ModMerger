#include "singlemod.h"

#include <QDataStream>

Mod::Mod(QString path, QObject* parent) : QObject(parent),  folderPath(path)
{
    using namespace Constants;

    folderPathNative = QDir::toNativeSeparators(folderPath);
    contentPath = folderPath + CONTENT_PFIX;
    scriptsPath = contentPath + SCRIPTS_PFIX;
    cachePath = contentPath + CACHE_PFIX;
    modName = folderPath.mid( folderPath.lastIndexOf(SLASH) + 1 );

    metadata.setFile(contentPath + METADATA_PFIX);
    metadata.parse();

    checker = new QFileInfo;

    checker->setFile(scriptsPath);
    hasScripts = checker->exists() && checker->isDir();

    checker->setFile(cachePath);
    hasCache = checker->exists() && checker->isFile() && checker->size() > 0;

    hasMetadata = metadata.isValid();

    if ( hasMetadata ) {
        for (QString bundleName : metadata.bundlesList) {
            bundles.append( new BundleFile( contentPath + SLASH + bundleName, this ) );
        }
    }

    hasBundles = bundles.size() > 0;

    QDir folder(contentPath);
    folder.setNameFilters(QStringList("*" + RENAME_PFIX));
    folder.setFilter(QDir::Files);
    mergedResources = folder.entryInfoList();

    modState = ( mergedResources.size() > 0 ) ? MERGED : NOT_MERGED;
    isMergeable = (hasMetadata && hasBundles) || modState == MERGED;

    folder.setNameFilters(QStringList({"*.bundle", "*.store", "*.cache"}));
    folder.setFilter(QDir::Files);
    QFileInfoList detectedResources = folder.entryInfoList();

    /* Some checks for notes */

    if ( mergedResources.size() > 0 && detectedResources.size() > 0) {
        notes.append( tr("Mod contains both merged and unmerged bundles, please delete unnecessary files. ", "Incorrect mod structure warning.") );
        modState = CORRUPTED;
    }

    if (isMergeable) {
        for (QString line : metadata.filesList) {
            if (line.indexOf(ICON_CHECK) != -1) {
                isMergeable = false;
                break;
            }

            if (line.indexOf(XML_CHECK) != -1) {
                notes.append( tr("XML files detected, merging is NOT recommended. ", "Warns about detected xmls.") );
                break;
            }

            if (line.indexOf(SWF_CHECK) != -1) {
                notes.append( tr("SWF files detected, merging is NOT recommended. ", "Warns about detected swfs.") );
                break;
            }
        }
    }
}

void Mod::renameMerge()
{
    using namespace Constants;

    checker->setFile(cachePath);

    if ( checker->exists() ) {
        QFile::rename(cachePath, cachePath + RENAME_PFIX);
    }

    if (hasMetadata) {
        QFile::rename(contentPath + METADATA_PFIX, contentPath + METADATA_PFIX + RENAME_PFIX);
    }

    if (hasBundles) {
        for (auto file : bundles) {
            QFile::rename(file->fullPath, file->fullPath + RENAME_PFIX);
        }
    }
}

void Mod::renameUnmerge()
{
    for (QFileInfo mr : mergedResources) {
        QString newName = mr.absoluteFilePath();
        newName.chop(Constants::RENAME_PFIX.size());
        QFile::rename( mr.absoluteFilePath(), newName );
    }

}
