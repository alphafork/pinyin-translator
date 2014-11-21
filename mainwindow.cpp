// Copyright (c) 2010-2014 Sami Väisänen, Ensisoft 
//
// http://www.ensisoft.com
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.

#include "config.h"

#include "warnpush.h"
#  include <QtGui/QKeyEvent>
#  include <QtGui/QClipboard>
#  include <QtGui/QStyleFactory>
#  include <QtGui/QMessageBox>
#  include <QtGui/QFontDialog>
#  include <QtDebug>
#  include <QDir>
#  include <QFileInfo>
#  include <QSettings>
#include "warnpop.h"
#include <stdexcept>
#include <algorithm>

#include "mainwindow.h"
#include "dlgword.h"
#include "dlgdictionary.h"

#define NOTE(x) \
    ui_.statusbar->showMessage(x, 5000)

namespace pime
{

class MainWindow::DicModel : public QAbstractTableModel
{
public:
    DicModel(dictionary& dic) : dic_(dic), traditional_(true)
    {}

    virtual QVariant data(const QModelIndex& index, int role) const override
    {
        const auto row = index.row();
        const auto col = index.column();
        if (role == Qt::DisplayRole)
        {
            const auto& word = *words_[row];
            switch (col)
            {
                case 0: 
                    if (row < 12)
                        return "F" + QString::number(row + 1);
                    else return "";
                case 1:
                    if (traditional_)
                        return word.traditional;
                    else return word.simplified;

                case 2: return word.pinyin;
                case 3: return word.description;
            }            
        }
        else if (role == Qt::FontRole)
        {
            if (col == 1)
                return chfont_;
        }
        return QVariant();        
    }

    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override
    {
        if (role == Qt::DisplayRole &&
            orientation == Qt::Horizontal)
        {
            switch (section)
            {
                case 0: return "Shortcut";
                case 1: return "Chinese";
                case 2: return "Pinyin";
                case 3: return "Definition";
            }
        }
        return QVariant();
    }
    virtual int rowCount(const QModelIndex&) const override
    { 
        return (int)words_.size();
    }
    virtual int columnCount(const QModelIndex&) const override
    {
        return 4;
    }
    std::size_t size() const 
    {
        return words_.size();
    }

    void store(dictionary::word word)
    {
        dic_.store(word);
    }
    void update(const QString& key)
    {
        words_ = dic_.lookup(key);
        reset();
    }
    const dictionary::word& getWord(std::size_t i) const 
    {
        return *words_[i];
    }

    void toggleTraditional(bool on_off)
    {
        traditional_ = on_off;
        auto first = QAbstractTableModel::index(0, 0);
        auto last  = QAbstractTableModel::index(words_.size(), 4);
        emit dataChanged(first, last);
    }

    void setChFont(QFont font)
    {
        chfont_ = font;
        auto first = QAbstractTableModel::index(0, 0);
        auto last  = QAbstractTableModel::index(words_.size(), 4);
        emit dataChanged(first, last);
    }
private:
    std::vector<const dictionary::word*> words_;
    dictionary& dic_;
    bool traditional_;
    QFont chfont_;
};

MainWindow::MainWindow() : model_(new DicModel(dic_))
{
    ui_.setupUi(this);
    ui_.tableView->setModel(model_.get());
    ui_.statusbar->insertPermanentWidget(0, ui_.frmInfo);

    const auto& home = QDir::homePath();
    const auto& pime = home + "/.pime/";
    const auto& local = pime + "local.dic";

    QDir dir(pime);
    if (!dir.mkpath(pime))
        throw std::runtime_error("failed to create ~/.pime");

    quint32 metaid = 1;
    meta data;
    data.file   = local;
    data.metaid = metaid;
    meta_.insert(std::make_pair(metaid, data));
    if (QFileInfo(local).exists())
    {
        dic_.load(local, metaid);
        qDebug() << "Loaded local dictionary " << local;
    }

    ++metaid;
    QStringList filter("*.dic");
    QStringList dics = dir.entryList(filter);
    for (const auto& file : dics)
    {
        if (file == "local.dic")
            continue;

        meta data;
        data.file = pime + file;
        data.metaid = metaid;
        qDebug() << "Loading: " << pime + file;

        dic_.load(pime + file, metaid);
        meta_.insert(std::make_pair(metaid, data));
        ++metaid;
    }

    NOTE(QString("Loaded dictionary with %1 words").arg(dic_.wordCount()));            

    QSettings settings("Ensisoft", "Pime");
    const auto wwidth  = settings.value("window/width", width()).toInt();
    const auto wheight = settings.value("window/height", height()).toInt();
    const auto xpos   = settings.value("window/xpos", x()).toInt();
    const auto ypos   = settings.value("window/ypos", y()).toInt();
    const auto traditional = settings.value("window/traditional", true).toBool();
    const auto font = settings.value("window/font").toString();
    if (!font.isEmpty())
    {
        QFont f;
        if (f.fromString(font))
            setFont(f);
    }

    ui_.actionTraditional->setChecked(traditional);
    ui_.actionSimplified->setChecked(!traditional);
    ui_.editInput->installEventFilter(this);    

    model_->toggleTraditional(traditional);

    move(xpos, ypos);
    resize(wwidth, wheight);

    updateDictionary("");
    updateWordCount();

    QStyle* style = QApplication::setStyle("Cleanlooks");
    if (style)
    {
        QApplication::setPalette(style->standardPalette());
    }
}

MainWindow::~MainWindow()
{
    QSettings settings("Ensisoft", "Pime");
    settings.setValue("window/width", width());
    settings.setValue("window/height", height());
    settings.setValue("window/xpos", x());
    settings.setValue("window/ypos", y());
    settings.setValue("window/traditional", ui_.actionTraditional->isChecked());
    settings.setValue("window/font", font_.toString());

    for (const auto& meta : meta_)
    {
        const auto& file = meta.second.file;
        const auto& metaid = meta.second.metaid;
        dic_.save(file, metaid);

        qDebug() << "Saving words to " << file;
    }
}

void MainWindow::on_actionExit_triggered()
{
    close();
}

void MainWindow::on_actionNewWord_triggered()
{
    const auto key = ui_.editInput->text();

    DlgWord dlg(this);
    if (dlg.exec() == QDialog::Rejected)
        return;

    dictionary::word word;
    //word.key         = dlg.key();
    word.traditional = dlg.traditional();
    word.simplified  = dlg.simplified();
    word.pinyin      = dlg.pinyin();
    word.description = dlg.desc();
    word.meta        = 1;
    word.erased      = false;
    dic_.store(word);
    model_->update(key);

    NOTE(QString("Added %1").arg(word.traditional));

    updateWordCount();
}

void MainWindow::on_actionNewText_triggered()
{
    ui_.editInput->clear();
    ui_.editInput->setFocus();

    line_.clear();
    updateTranslation();    
    updateDictionary("");
}

void MainWindow::on_actionDictionary_triggered()
{
    DlgDictionary dlg(font_, this, dic_);
    dlg.exec();

    updateWordCount();
}

void MainWindow::on_actionTraditional_triggered()
{
    ui_.actionSimplified->setChecked(false);

    model_->toggleTraditional(true);

    updateTranslation();
}

void MainWindow::on_actionSimplified_triggered()
{
    ui_.actionTraditional->setChecked(false);

    model_->toggleTraditional(false);

    updateTranslation();
}


void MainWindow::on_actionAbout_triggered()
{
    QMessageBox msg(this);
    msg.setWindowTitle(QString("Aout %1").arg(windowTitle()));
    msg.setText(QString::fromUtf8(
        "Pinyin-Translator v0.1\n\n"
        "Copyright (c) 2014 Sami Väisänen\n"
        "http://www.ensisoft.com/\n\n"
        "CC-CEDICT\n"
        "Community maintained free Chinese-English dictionary\n"
        "Published by MDBG\n"
        "Creative Commons Attribution-Share Alike 3.0\n"
        "http://createtivecommons.org/licenses/by-sa/3.0/\n"
        "CEDICT - Copyright (c) 1997, 1998 Paul Andrew Denisowski\n"));
    msg.setStandardButtons(QMessageBox::Ok);
    msg.setIcon(QMessageBox::Information);
    msg.exec();
}

void MainWindow::on_actionFont_triggered()
{
    QFontDialog dlg(font_, this);
    if (dlg.exec() == QDialog::Rejected)
        return;

    QFont font = dlg.selectedFont();
    setFont(font);
}

void MainWindow::on_editInput_textEdited(const QString& text)
{
    updateDictionary(text);
}

void MainWindow::on_tableView_doubleClicked(const QModelIndex& index)
{
    const auto input = ui_.editInput->text();
    if (input.isEmpty())
        return;

    const auto row   = index.row();

    translate(row, input);
    updateTranslation();
    updateDictionary("");

    ui_.editInput->clear();
    ui_.editInput->setFocus();
}

bool MainWindow::eventFilter(QObject* receiver, QEvent* event)
{
    if (receiver != ui_.editInput)
        return QMainWindow::eventFilter(receiver, event);
    if (event->type() != QEvent::KeyPress)
        return QMainWindow::eventFilter(receiver, event);        

    const auto* keypress = static_cast<const QKeyEvent*>(event);
    const auto input = ui_.editInput->text();

    int wordindex = 0;
    bool copy_to_clip = false;
    switch (keypress->key())
    {
        case Qt::Key_F1: wordindex = 1; break;
        case Qt::Key_F2: wordindex = 2; break;
        case Qt::Key_F3: wordindex = 3; break;
        case Qt::Key_F4: wordindex = 4; break;
        case Qt::Key_F5: wordindex = 5; break;
        case Qt::Key_F6: wordindex = 6; break;
        case Qt::Key_F7: wordindex = 7; break;
        case Qt::Key_F8: wordindex = 8; break;
        case Qt::Key_F9: wordindex = 9; break;
        case Qt::Key_F10: wordindex = 10; break;
        case Qt::Key_F11: wordindex = 11; break;
        case Qt::Key_F12: wordindex = 12; break;
        case Qt::Key_Space: wordindex = 1; break;
        case Qt::Key_Enter: wordindex = 1; copy_to_clip = true; break;
    }

    if (!wordindex)
        return QMainWindow::eventFilter(receiver, event);

    qDebug() << "List index: " << wordindex;
    qDebug() << "Input word: " << input;

    translate(wordindex-1, input);                
    updateTranslation();
    updateDictionary("");

    ui_.editInput->clear();

    if (copy_to_clip)
    {
        QString chinese = ui_.editChinese->text();
        QClipboard* clip = QApplication::clipboard();
        clip->setText(chinese);
    }
    return true;
}

void MainWindow::translate(int index, const QString& key)
{
    if (index >= model_->size())
    {
        word w;
        w.pinyin      = key;
        w.traditional = key;
        w.simplified  = key;
        line_.push_back(w);
    }
    else
    {
        const auto& translate = model_->getWord(index);
        word w;
        w.pinyin      = translate.pinyin;
        w.traditional = translate.traditional;
        w.simplified  = translate.simplified;
        line_.push_back(w);
    }
}

void MainWindow::updateDictionary(const QString& key)
{
    qDebug() << "Dictionary key: " << key;

    model_->update(key);

//     ui_.listWords->clear();
//
//     words_ = dic_.lookup(key);
//     for (size_t i=0; i<words_.size(); ++i)
//     {
//         const auto& word = words_[i];
//
//         QListWidgetItem* item = new QListWidgetItem();
//         item->setText(QString("%1.\t%2\t%3\t%4")
//             .arg(i + 1)
//             .arg(word.chinese)
//             .arg(word.pinyin)
//             .arg(word.description));
//         ui_.listWords->addItem(item);
//     }
}

void MainWindow::updateTranslation()
{
    bool simplified = ui_.actionSimplified->isChecked();

    QString chinese;
    QString pinyin;
    for (const auto& tok : line_)
    {
        if (simplified)
            chinese += tok.simplified;
        else chinese += tok.traditional;
        pinyin  += QString("%1 ").arg(tok.pinyin);
    }

    ui_.editPinyin->setText(pinyin);
    ui_.editChinese->setText(chinese);
}

void MainWindow::updateWordCount()
{
    ui_.lblInfoText->setText(
        QString("%1 words").arg(dic_.wordCount()));
}

void MainWindow::setFont(QFont font)
{
    //ui_.editInput->setFont(font);
    //ui_.editPinyin->setFont(font);
    ui_.editChinese->setFont(font);
    model_->setChFont(font);
    //ui_.tableView->setFont(font);

    qDebug() << "Font set to: " << font.rawName() << " " << font.pixelSize() << " px";

    font_ = font;
}

} // pime