#pragma once

#include <QMainWindow>
#include <QProcess>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QVector>
#include <QJsonValue>

class QDragEnterEvent;
class QDragMoveEvent;
class QDropEvent;

struct MetadataRow {
    QString source;       // exiftool / xattr
    QString key;          // e.g. EXIF:DateTimeOriginal / com.apple.quarantine
    QString value;        // display value
    bool removable;       // whether the row can be selected and removed
    QString removeArg;    // exiftool tag arg or xattr name
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void onClearClicked();
    void onSelectAllRemovableClicked();
    void onRefreshClicked();

private:
    QLabel *dropHintLabel = nullptr;
    QLabel *fileLabel = nullptr;
    QLabel *statusLabel = nullptr;
    QTableWidget *table = nullptr;
    QPushButton *clearButton = nullptr;
    QPushButton *selectAllButton = nullptr;
    QPushButton *refreshButton = nullptr;

    QString currentFilePath;
    QVector<MetadataRow> rows;

    void buildUi();
    void loadFile(const QString &filePath);
    void refreshMetadata();
    void renderRows();
    void setStatus(const QString &message, bool isError = false);

    QString findExifTool() const;
    bool runProcess(const QString &program,
                    const QStringList &arguments,
                    QString *stdOut,
                    QString *stdErr,
                    int timeoutMs = 20000) const;

    QVector<MetadataRow> readExifToolMetadata(const QString &filePath) const;
    QVector<MetadataRow> readMacExtendedAttributes(const QString &filePath) const;

    QString normalizeValueForDisplay(const QJsonValue &value) const;
    bool isExifToolTagRemovable(const QString &tagKey) const;
};
