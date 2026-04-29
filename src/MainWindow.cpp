#include "MainWindow.h"

#include <QApplication>
#include <QBoxLayout>
#include <QCoreApplication>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QMimeData>
#include <QStandardPaths>
#include <QUrl>
#include <QFileInfo>
#include <QSet>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    buildUi();
    setAcceptDrops(true);
    setWindowTitle("FileMeta Cleaner");
}

void MainWindow::buildUi() {
    auto *central = new QWidget(this);
    auto *root = new QVBoxLayout(central);
    root->setContentsMargins(18, 18, 18, 18);
    root->setSpacing(12);

    dropHintLabel = new QLabel("将文件拖到此窗口中，应用会读取并展示可识别的文件元信息。", central);
    dropHintLabel->setAlignment(Qt::AlignCenter);
    dropHintLabel->setMinimumHeight(84);
    dropHintLabel->setStyleSheet(
        "QLabel {"
        " border: 2px dashed #999;"
        " border-radius: 12px;"
        " color: #555;"
        " font-size: 16px;"
        " padding: 18px;"
        " background: #fafafa;"
        "}"
    );
    root->addWidget(dropHintLabel);

    fileLabel = new QLabel("当前文件：未选择", central);
    fileLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    root->addWidget(fileLabel);

    table = new QTableWidget(0, 5, central);
    table->setHorizontalHeaderLabels({"选择", "来源", "元信息名称", "值", "状态"});
    table->horizontalHeader()->setStretchLastSection(false);
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    table->verticalHeader()->setVisible(false);
    table->setAlternatingRowColors(true);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    root->addWidget(table, 1);

    auto *buttonLayout = new QHBoxLayout();
    selectAllButton = new QPushButton("勾选可清除项", central);
    refreshButton = new QPushButton("重新读取", central);
    clearButton = new QPushButton("清除", central);
    clearButton->setDefault(true);
    buttonLayout->addWidget(selectAllButton);
    buttonLayout->addWidget(refreshButton);
    buttonLayout->addStretch(1);
    buttonLayout->addWidget(clearButton);
    root->addLayout(buttonLayout);

    statusLabel = new QLabel("提示：应用打包后会优先使用内置 ExifTool，用户无需单独安装 Homebrew 或 ExifTool。", central);
    statusLabel->setWordWrap(true);
    root->addWidget(statusLabel);

    setCentralWidget(central);

    connect(clearButton, &QPushButton::clicked, this, &MainWindow::onClearClicked);
    connect(selectAllButton, &QPushButton::clicked, this, &MainWindow::onSelectAllRemovableClicked);
    connect(refreshButton, &QPushButton::clicked, this, &MainWindow::onRefreshClicked);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void MainWindow::dragMoveEvent(QDragMoveEvent *event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *event) {
    const auto urls = event->mimeData()->urls();
    for (const QUrl &url : urls) {
        if (url.isLocalFile()) {
            loadFile(url.toLocalFile());
            event->acceptProposedAction();
            return;
        }
    }
    setStatus("未识别到本地文件。", true);
}

void MainWindow::loadFile(const QString &filePath) {
    QFileInfo info(filePath);
    if (!info.exists() || !info.isFile()) {
        setStatus("请拖入一个有效文件。", true);
        return;
    }
    currentFilePath = filePath;
    fileLabel->setText("当前文件：" + currentFilePath);
    refreshMetadata();
}

void MainWindow::onRefreshClicked() {
    if (currentFilePath.isEmpty()) {
        setStatus("请先拖入文件。", true);
        return;
    }
    refreshMetadata();
}

void MainWindow::refreshMetadata() {
    rows.clear();

    if (currentFilePath.isEmpty()) {
        renderRows();
        return;
    }

    rows += readExifToolMetadata(currentFilePath);
    rows += readMacExtendedAttributes(currentFilePath);

    renderRows();
    setStatus(QString("已读取 %1 项元信息。可清除项可以勾选后点击“清除”。").arg(rows.size()));
}

void MainWindow::renderRows() {
    table->setRowCount(rows.size());

    for (int r = 0; r < rows.size(); ++r) {
        const MetadataRow &row = rows.at(r);

        auto *check = new QCheckBox(table);
        check->setEnabled(row.removable);
        auto *checkWrap = new QWidget(table);
        auto *checkLayout = new QHBoxLayout(checkWrap);
        checkLayout->setContentsMargins(0, 0, 0, 0);
        checkLayout->setAlignment(Qt::AlignCenter);
        checkLayout->addWidget(check);
        table->setCellWidget(r, 0, checkWrap);

        auto *sourceItem = new QTableWidgetItem(row.source);
        auto *keyItem = new QTableWidgetItem(row.key);
        auto *valueItem = new QTableWidgetItem(row.value);
        auto *statusItem = new QTableWidgetItem(row.removable ? "可清除" : "只读/不建议清除");

        if (!row.removable) {
            sourceItem->setFlags(sourceItem->flags() & ~Qt::ItemIsEnabled);
            keyItem->setFlags(keyItem->flags() & ~Qt::ItemIsEnabled);
            valueItem->setFlags(valueItem->flags() & ~Qt::ItemIsEnabled);
            statusItem->setFlags(statusItem->flags() & ~Qt::ItemIsEnabled);
        }

        table->setItem(r, 1, sourceItem);
        table->setItem(r, 2, keyItem);
        table->setItem(r, 3, valueItem);
        table->setItem(r, 4, statusItem);
    }
}

void MainWindow::onSelectAllRemovableClicked() {
    for (int r = 0; r < table->rowCount(); ++r) {
        if (!rows.at(r).removable) {
            continue;
        }
        auto *wrap = table->cellWidget(r, 0);
        if (!wrap) {
            continue;
        }
        auto *check = wrap->findChild<QCheckBox *>();
        if (check) {
            check->setChecked(true);
        }
    }
}

void MainWindow::onClearClicked() {
    if (currentFilePath.isEmpty()) {
        setStatus("请先拖入文件。", true);
        return;
    }

    QStringList exifArgs;
    QStringList xattrs;

    for (int r = 0; r < table->rowCount(); ++r) {
        auto *wrap = table->cellWidget(r, 0);
        auto *check = wrap ? wrap->findChild<QCheckBox *>() : nullptr;
        if (!check || !check->isChecked()) {
            continue;
        }

        const MetadataRow &row = rows.at(r);
        if (!row.removable) {
            continue;
        }

        if (row.source == "exiftool") {
            exifArgs << row.removeArg;
        } else if (row.source == "xattr") {
            xattrs << row.removeArg;
        }
    }

    if (exifArgs.isEmpty() && xattrs.isEmpty()) {
        setStatus("请至少勾选一项可清除元信息。", true);
        return;
    }

    QString errors;

    if (!exifArgs.isEmpty()) {
        const QString exiftool = findExifTool();
        if (exiftool.isEmpty()) {
            errors += "未找到 ExifTool。请确认打包时已执行 scripts/prepare_exiftool.sh，并且 .app 内包含 Contents/Resources/exiftool/exiftool。\n";
        } else {
            QString out, err;
            QStringList args;
            args << "-overwrite_original";
            args << exifArgs;
            args << currentFilePath;

            if (!runProcess(exiftool, args, &out, &err, 60000)) {
                errors += "ExifTool 执行失败：" + err + "\n";
            }
        }
    }

    for (const QString &attr : xattrs) {
        QString out, err;
        const bool ok = runProcess("/usr/bin/xattr", {"-d", attr, currentFilePath}, &out, &err, 20000);
        if (!ok) {
            errors += QString("xattr 删除失败 [%1]：%2\n").arg(attr, err.trimmed());
        }
    }

    refreshMetadata();

    if (errors.isEmpty()) {
        QMessageBox::information(this, "完成", "已清除所选元信息。建议打开文件确认内容是否正常。重要文件请先备份。");
        setStatus("已清除所选元信息。");
    } else {
        QMessageBox::warning(this, "部分失败", errors);
        setStatus("部分元信息清除失败，请查看提示。", true);
    }
}

QString MainWindow::findExifTool() const {
    // 1. 打包后的优先路径：
    // FileMetaCleaner.app/Contents/Resources/exiftool/exiftool
    QDir appDir(QCoreApplication::applicationDirPath());

    // QCoreApplication::applicationDirPath() 在 .app 内一般是：xxx.app/Contents/MacOS
    if (appDir.cdUp()) {
        const QString bundled = appDir.filePath("Resources/exiftool/exiftool");
        const QFileInfo bundledInfo(bundled);
        if (bundledInfo.exists() && bundledInfo.isFile() && bundledInfo.isExecutable()) {
            return bundled;
        }
    }

    // 2. 开发调试兜底：从系统 PATH 查找
    const QString direct = QStandardPaths::findExecutable("exiftool");
    if (!direct.isEmpty()) {
        return direct;
    }

    // 3. Finder 启动的 GUI App 通常拿不到 shell PATH，所以再兜底查常见路径
    const QStringList commonPaths = {
        "/opt/homebrew/bin/exiftool", // Apple Silicon Homebrew
        "/usr/local/bin/exiftool",    // Intel Homebrew
        "/usr/bin/exiftool"
    };
    for (const QString &path : commonPaths) {
        const QFileInfo info(path);
        if (info.exists() && info.isFile() && info.isExecutable()) {
            return path;
        }
    }
    return {};
}

bool MainWindow::runProcess(const QString &program,
                            const QStringList &arguments,
                            QString *stdOut,
                            QString *stdErr,
                            int timeoutMs) const {
    QProcess process;
    process.setProgram(program);
    process.setArguments(arguments);
    process.start();

    if (!process.waitForStarted(5000)) {
        if (stdErr) {
            *stdErr = "进程无法启动：" + program;
        }
        return false;
    }

    if (!process.waitForFinished(timeoutMs)) {
        process.kill();
        process.waitForFinished(3000);
        if (stdErr) {
            *stdErr = "进程执行超时：" + program;
        }
        return false;
    }

    if (stdOut) {
        *stdOut = QString::fromUtf8(process.readAllStandardOutput());
    }
    if (stdErr) {
        *stdErr = QString::fromUtf8(process.readAllStandardError());
    }

    return process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
}

QVector<MetadataRow> MainWindow::readExifToolMetadata(const QString &filePath) const {
    QVector<MetadataRow> result;
    const QString exiftool = findExifTool();
    if (exiftool.isEmpty()) {
        result.push_back({"exiftool", "ExifTool", "未找到内置 ExifTool。请确认打包步骤已正确执行。", false, {}});
        return result;
    }

    QString out, err;
    const bool ok = runProcess(exiftool, {"-json", "-a", "-G1", "-s", filePath}, &out, &err, 30000);
    if (!ok || out.trimmed().isEmpty()) {
        result.push_back({"exiftool", "读取失败", err.trimmed(), false, {}});
        return result;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(out.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isArray() || doc.array().isEmpty()) {
        result.push_back({"exiftool", "解析失败", parseError.errorString(), false, {}});
        return result;
    }

    const QJsonObject obj = doc.array().first().toObject();
    const QStringList keys = obj.keys();

    for (const QString &key : keys) {
        const QString value = normalizeValueForDisplay(obj.value(key));
        const bool removable = isExifToolTagRemovable(key);
        result.push_back({"exiftool", key, value, removable, "-" + key + "="});
    }

    return result;
}

QVector<MetadataRow> MainWindow::readMacExtendedAttributes(const QString &filePath) const {
    QVector<MetadataRow> result;
    QString out, err;

    const bool ok = runProcess("/usr/bin/xattr", {filePath}, &out, &err, 20000);
    if (!ok && !err.trimmed().isEmpty()) {
        result.push_back({"xattr", "读取失败", err.trimmed(), false, {}});
        return result;
    }

    const QStringList attrs = out.split('\n', Qt::SkipEmptyParts);
    for (const QString &attrRaw : attrs) {
        const QString attr = attrRaw.trimmed();
        if (attr.isEmpty()) {
            continue;
        }

        QString valueOut, valueErr;
        runProcess("/usr/bin/xattr", {"-p", attr, filePath}, &valueOut, &valueErr, 20000);
        QString value = valueOut.trimmed();
        if (value.isEmpty() && !valueErr.trimmed().isEmpty()) {
            value = valueErr.trimmed();
        }
        if (value.size() > 500) {
            value = value.left(500) + " ...";
        }

        result.push_back({"xattr", attr, value, true, attr});
    }

    return result;
}

QString MainWindow::normalizeValueForDisplay(const QJsonValue &value) const {
    if (value.isString()) {
        return value.toString();
    }
    if (value.isDouble()) {
        return QString::number(value.toDouble());
    }
    if (value.isBool()) {
        return value.toBool() ? "true" : "false";
    }
    if (value.isArray()) {
        return QString::fromUtf8(QJsonDocument(value.toArray()).toJson(QJsonDocument::Compact));
    }
    if (value.isObject()) {
        return QString::fromUtf8(QJsonDocument(value.toObject()).toJson(QJsonDocument::Compact));
    }
    return QString();
}

bool MainWindow::isExifToolTagRemovable(const QString &tagKey) const {
    if (tagKey == "SourceFile") {
        return false;
    }

    const QString lower = tagKey.toLower();
    static const QStringList readonlyPrefixes = {
        "file:",       // FileName, Directory, FileSize, FileModifyDate, MIMEType...
        "system:",
        "composite:",  // Derived values, not directly stored tags
        "exiftool:"
    };
    for (const QString &prefix : readonlyPrefixes) {
        if (lower.startsWith(prefix)) {
            return false;
        }
    }

    // Some tags may still be read-only depending on file type; exiftool will report failure then.
    return true;
}

void MainWindow::setStatus(const QString &message, bool isError) {
    statusLabel->setText(message);
    statusLabel->setStyleSheet(isError ? "color: #b00020;" : "color: #444;");
}
