#include "projectpropertiesdialog.h"
#include "ui_projectpropertiesdialog.h"

#include "dbg_cnesmappers.h"

#include "cnessystempalette.h"

#include "propertyeditordialog.h"

#include "nes_emulator_core.h"

#include "main.h"

const char hexStr[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

ProjectPropertiesDialog::ProjectPropertiesDialog(QWidget* parent) :
   QDialog(parent),
   ui(new Ui::ProjectPropertiesDialog)
{
   QList<QColor> *pal = nesicideProject->getProjectPaletteEntries();
   int i = 0;
   char mapperTag [ 64 ];

   linkerConfigChanged = false;
   customRulesChanged = false;

   // Initialize UI elements...
   ui->setupUi(this);

   // Common project properties.
   ui->projectNameLineEdit->setText(nesicideProject->getProjectTitle());
   ui->projectBasePath->setText(QDir::fromNativeSeparators(QDir::currentPath()));
   ui->projectOutputBasePath->setText(nesicideProject->getProjectOutputBasePath());
   ui->outputName->setText(nesicideProject->getProjectOutputName());
   ui->compilerDefinedSymbols->setText(nesicideProject->getCompilerDefinedSymbols());
   ui->compilerIncludePaths->setText(nesicideProject->getCompilerIncludePaths());
   ui->compilerAdditionalOptions->setText(nesicideProject->getCompilerAdditionalOptions());
   ui->assemblerDefinedSymbols->setText(nesicideProject->getAssemblerDefinedSymbols());
   ui->assemblerIncludePaths->setText(nesicideProject->getAssemblerIncludePaths());
   ui->assemblerAdditionalOptions->setText(nesicideProject->getAssemblerAdditionalOptions());
   ui->linkerOutputName->setText(nesicideProject->getProjectLinkerOutputName());
   ui->debugInfoName->setText(nesicideProject->getProjectDebugInfoName());
   ui->linkerAdditionalOptions->setText(nesicideProject->getLinkerAdditionalOptions());
   ui->linkerAdditionalDependencies->setText(nesicideProject->getLinkerAdditionalDependencies());
   ui->linkerConfigFile->setText(nesicideProject->getLinkerConfigFile());
   deserializeLinkerConfig();
   ui->customRuleFile->setText(nesicideProject->getMakefileCustomRulesFile());
   deserializeCustomRules();

   if ( ui->linkerConfigFile->text().isEmpty() )
   {
      ui->linkerConfig->setText("Select a file to store linker configuration in, then modify it here.");
      ui->linkerConfig->setEnabled(false);
   }
   if ( ui->customRuleFile->text().isEmpty() )
   {
      ui->customRules->setText("Select a file to store custom makefile rules in, then modify it here.");
      ui->customRules->setEnabled(false);
   }

   // NES-specific project properties.
   ui->projectHeaderName->setText(nesicideProject->getProjectHeaderFileName());
   ui->projectSourceName->setText(nesicideProject->getProjectSourceFileName());
   ui->prgromOutputBasePath->setText(nesicideProject->getProjectLinkerOutputBasePath());
   ui->chrromOutputBasePath->setText(nesicideProject->getProjectCHRROMOutputBasePath());
   ui->chrromOutputName->setText(nesicideProject->getProjectCHRROMOutputName());
   ui->chrRom->setChecked(nesicideProject->getProjectUsesCHRROM());
   ui->chrRam->setChecked(!nesicideProject->getProjectUsesCHRROM());
   ui->cartridgeOutputName->setText(nesicideProject->getProjectCartridgeOutputName());
   ui->cartridgeSaveStateName->setText(nesicideProject->getProjectCartridgeSaveStateName());
   ui->trainerPresent->setChecked(false);
   ui->saveRAMPresent->setChecked(nesicideProject->getCartridge()->isBatteryBackedRam());
   ui->extraVRAMPresent->setChecked(nesicideProject->getCartridge()->getFourScreen());

   // C64-specific project properties.

   // Link up the project palette to this dialog
   for (int paletteItemIndex=0; paletteItemIndex<pal->count(); paletteItemIndex++)
   {
      currentPalette.append(pal->at(paletteItemIndex));
   }

   ui->tableWidget->setItemDelegate(new CPaletteItemDelegate(this));

   for (int row=0; row <= 0x3; row++)
   {
      for (int col=0; col <= 0xF; col++)
      {
         if (row == 0)
         {
            ui->tableWidget->setColumnWidth(col, 25);
         }

         ui->tableWidget->setItem(row, col, new QTableWidgetItem(QString(hexStr[row] + QString(hexStr[col]))));
         ui->tableWidget->item(row, col)->setTextAlignment(Qt::AlignCenter);
         ui->tableWidget->item(row, col)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
      }
   }

   // Create mapper entries in mapper combo box
   i = 0;

   while ( mapper[i].name )
   {
      sprintf ( mapperTag, "%3d:%s", mapper[i].id, mapper[i].name );
      ui->mapperComboBox->insertItem ( i , mapperTag );
      i++;
   }

   CCartridge* pCartridge = nesicideProject->getCartridge();
   ui->mapperComboBox->setCurrentIndex(mapperIndexFromID(pCartridge->getMapperNumber()));
   ui->romTypeComboBox->setCurrentIndex(0);
   ui->mirroringComboBox->setCurrentIndex(pCartridge->getMirrorMode());

   tilePropertyListModel = new CPropertyListModel(false);
   tilePropertyListModel->setItems(nesicideProject->getTileProperties());
   ui->propertyTableView->setModel(tilePropertyListModel);

   pageMap.insert("Project",ui->project);
   pageMap.insert("Compiler",ui->compiler);
   pageMap.insert("Assembler",ui->assembler);
   pageMap.insert("Linker",ui->linker);
   pageMap.insert("Custom Rules",ui->customrules);
   pageMap.insert("Nintendo Entertainment System",ui->nesgraphicsbuilder);
   pageMap.insert("Graphics Builder",ui->nesgraphicsbuilder);
   pageMap.insert("Cartridge",ui->nescartridge);
   pageMap.insert("Tile Properties",ui->nestileproperties);
   pageMap.insert("System Palette",ui->nessystempalette);

   ui->treeWidget->setCurrentItem(ui->treeWidget->findItems("Project",Qt::MatchExactly).at(0));
   if ( !nesicideProject->getProjectTarget().compare("nes",Qt::CaseInsensitive) )
   {
      QList<QTreeWidgetItem*> items = ui->treeWidget->findItems("Commodore 64",Qt::MatchExactly);
      if ( items.count() )
      {
         items.at(0)->setHidden(true);
      }
   }
   else if ( !nesicideProject->getProjectTarget().compare("c64",Qt::CaseInsensitive) )
   {
      QList<QTreeWidgetItem*> items = ui->treeWidget->findItems("Nintendo Entertainment System",Qt::MatchExactly);
      if ( items.count() )
      {
         items.at(0)->setHidden(true);
      }
   }
   updateUI();
}

ProjectPropertiesDialog::~ProjectPropertiesDialog()
{
   delete tilePropertyListModel;
   delete ui;
}

void ProjectPropertiesDialog::changeEvent(QEvent* e)
{
   QDialog::changeEvent(e);

   switch (e->type())
   {
      case QEvent::LanguageChange:
         ui->retranslateUi(this);
         break;
      default:
         break;
   }
}


void ProjectPropertiesDialog::updateUI(int colid)
{
   for (int i=0; i<=0x3F; i++)
   {
      QTableWidgetItem* item = ui->tableWidget->item(i >> 4, i & 0xF);
      item->setBackgroundColor(currentPalette.value(i));

      if ((((double)currentPalette.value(i).red() +
            (double)currentPalette.value(i).green() +
            (double)currentPalette.value(i).blue()) / (double)3) < 200)
      {
         item->setForeground(QBrush(QColor(255, 255, 255), Qt::SolidPattern));
      }
      else
      {
         item->setForeground(QBrush(QColor(0, 0, 0), Qt::SolidPattern));
      }

   }

   ui->tableWidget->update();

   if (ui->tableWidget->selectedItems().isEmpty())
   {
      return;
   }

   QTableWidgetItem* selectedItem = ui->tableWidget->selectedItems().first();
   int selectedIdx = selectedItem->text().toInt(0, 16);

   ui->colorValueLabel->setText("#" +
                                QString(hexStr[(currentPalette.at(selectedIdx).red() >> 4) & 0xF]) +
                                QString(hexStr[currentPalette.at(selectedIdx).red() & 0xF]) +
                                QString(hexStr[(currentPalette.at(selectedIdx).green() >> 4) & 0xF]) +
                                QString(hexStr[currentPalette.at(selectedIdx).green() & 0xF]) +
                                QString(hexStr[(currentPalette.at(selectedIdx).blue() >> 4) & 0xF]) +
                                QString(hexStr[currentPalette.at(selectedIdx).blue() & 0xF]));

   if (colid != 1)
   {
      ui->redHorizontalSlider->setValue(currentPalette.at(selectedIdx).red());
   }

   if (colid != 2)
   {
      ui->greenHorizontalSlider->setValue(currentPalette.at(selectedIdx).green());
   }

   if (colid != 3)
   {
      ui->blueHorizontalSlider->setValue(currentPalette.at(selectedIdx).blue());
   }

}

void ProjectPropertiesDialog::on_tableWidget_cellClicked(int, int)
{
   updateUI();
}


void ProjectPropertiesDialog::on_resetPalettePushButton_clicked()
{
   // We are resetting the palette so we need to loop through and set our default palette structure's values
   // to the table.
   for (int col=0; col <= 0xF; col++)
   {
      for (int row=0; row <= 0x3; row++)
      {
         currentPalette.replace((row << 4) + col, QColor(nesGetPaletteRedComponent((row << 4)+col),
                                nesGetPaletteGreenComponent((row << 4)+col),
                                nesGetPaletteBlueComponent((row << 4)+col)));
      }
   }

   updateUI();
}

void ProjectPropertiesDialog::on_exportPalettePushButton_clicked()
{
   // Allow the user to select a file name. Note that using the static function produces a native
   // file dialog, while creating an instance of QFileDialog results in a non-native file dialog..
   QString fileName = QFileDialog::getSaveFileName(this, "Export Palette", QDir::currentPath(),
                                                   "NESICIDE Palette (*.npf)");

   if (!fileName.isEmpty())
   {
      // Create the XML document to save our palette into
      QDomDocument doc;

      // And the xml header to make it a valid xml document
      QDomProcessingInstruction instr = doc.createProcessingInstruction("xml", "version='1.0' encoding='UTF-8'");
      doc.appendChild(instr);

      // Create the root element, and give it a version attribute
      QDomElement rootElement = addElement( doc, doc, "nesicidepalette" );
      rootElement.setAttribute("version", 1.0);

      // Loop through all palette entries, and for each entry add an <entry /> tag that has the
      // index, as well as the RGB properties of the palette.
      for (int i=0; i <= 0x3F; i++)
      {
         QDomElement elm = addElement( doc, rootElement, "entry");
         elm.setAttribute("index", i);
         elm.setAttribute("r", ui->tableWidget->item(i >> 8, i & 0xF)->backgroundColor().red());
         elm.setAttribute("g", ui->tableWidget->item(i >> 8, i & 0xF)->backgroundColor().green());
         elm.setAttribute("b", ui->tableWidget->item(i >> 8, i & 0xF)->backgroundColor().blue());
      }

      // Create, and try to open a file object; we will use this to save the xml document we just generated.
      QFile file(fileName);

      if ( !file.open( QFile::WriteOnly) )
      {
         return;
      }

      // Create a text stream so we can stream the XML data to the file easily.
      QTextStream ts( &file );

      // Use the standard C++ stream function for streaming the string representation of our XML to
      // our file stream.
      ts << doc.toString();

      // And finally close the file.
      file.close();
   }
}

void ProjectPropertiesDialog::on_ImportPalettePushButton_clicked()
{
   // Allow the user to select a file name. Note that using the static function produces a native
   // file dialog, while creating an instance of QFileDialog results in a non-native file dialog..
   QString fileName = QFileDialog::getOpenFileName(this,"Import Palette",QDir::currentPath(),
                      "NESICIDE Palette (*.npf)");

   if (!fileName.isEmpty())
   {
      QDomDocument doc( "nesicidepalette" );
      QFile file(fileName);

      if ( !file.open(QFile::ReadOnly))
      {
         QMessageBox::information(0, "Error", "Error opening palette file.");
         return;
      }

      if ( !doc.setContent( &file ) )
      {
         QMessageBox::information(0, "Error", "Could not parse palette file.");
         file.close();
         return;
      }

      file.close();

      QDomElement root = doc.documentElement();

      if ( root.tagName() != "nesicidepalette" )
      {
         QMessageBox::information(0, "Error", "Invalid root in palette file.");
         return;
      }

      QDomNode n = root.firstChild();

      while ( !n.isNull() )
      {
         QDomElement e = n.toElement();

         if ( !e.isNull() )
         {
            if (e.tagName() == "entry")
            {
               int idx = e.attribute("index", "" ).toInt();
               int r = e.attribute("r", "").toInt();
               int g = e.attribute("g", "").toInt();
               int b = e.attribute("b", "").toInt();

               if ((idx <= 0x3F) && ui->tableWidget->item(idx >> 8, idx & 0xF))
               {
                  ui->tableWidget->item(idx >> 8, idx & 0xF)->setBackgroundColor(QColor(r, g, b));

                  if ((((double)r + (double)g + (double)b) / (double)3) < 200)
                     ui->tableWidget->item(idx >> 8, idx & 0xF)->setForeground(
                        QBrush(QColor(255, 255, 255), Qt::SolidPattern));
                  else
                     ui->tableWidget->item(idx >> 8, idx & 0xF)->setForeground(
                        QBrush(QColor(0, 0, 0), Qt::SolidPattern));
               }
            }
         }

         n = n.nextSibling();
      }

      ui->tableWidget->update();

   }
}

void ProjectPropertiesDialog::on_redHorizontalSlider_actionTriggered(int action)
{
   action = action;
   // Try to get the selected item
   QList<QTableWidgetItem *> sel = ui->tableWidget->selectedItems();
   QTableWidgetItem* item = sel.isEmpty() ? 0 : sel.first();

   // Don't do anything if we changed to an invalid item
   if (!item)
   {
      return;
   }

   int selectedIdx = item->text().toInt(0, 16);

   // Set the background color of the selected table cell to the values of our rgb dials
   currentPalette.replace(selectedIdx, QColor(ui->redHorizontalSlider->value(),
                          ui->greenHorizontalSlider->value(),
                          ui->blueHorizontalSlider->value()));

   // Refresh the user interface
   updateUI(1);
}

void ProjectPropertiesDialog::on_greenHorizontalSlider_actionTriggered(int action)
{
   action = action;
   // Try to get the selected item
   QList<QTableWidgetItem *> sel = ui->tableWidget->selectedItems();
   QTableWidgetItem* item = sel.isEmpty() ? 0 : sel.first();

   // Don't do anything if we changed to an invalid item
   if (!item)
   {
      return;
   }

   int selectedIdx = item->text().toInt(0, 16);

   // Set the background color of the selected table cell to the values of our rgb dials
   currentPalette.replace(selectedIdx, QColor(ui->redHorizontalSlider->value(),
                          ui->greenHorizontalSlider->value(),
                          ui->blueHorizontalSlider->value()));

   // Refresh the user interface
   updateUI(2);
}

void ProjectPropertiesDialog::on_blueHorizontalSlider_actionTriggered(int /*action*/)
{
   // Try to get the selected item
   QList<QTableWidgetItem *> sel = ui->tableWidget->selectedItems();
   QTableWidgetItem* item = sel.isEmpty() ? 0 : sel.first();

   // Don't do anything if we changed to an invalid item
   if (!item)
   {
      return;
   }

   int selectedIdx = item->text().toInt(0, 16);

   // Set the background color of the selected table cell to the values of our rgb dials
   currentPalette.replace(selectedIdx, QColor(ui->redHorizontalSlider->value(),
                          ui->greenHorizontalSlider->value(),
                          ui->blueHorizontalSlider->value()));

   // Refresh the user interface
   updateUI(3);
}

void ProjectPropertiesDialog::on_compilerIncludePathBrowse_clicked()
{
   QString value = QFileDialog::getExistingDirectory(this,"Additional Include Path",QDir::currentPath());
   QString includes = ui->compilerIncludePaths->text();
   QDir dir(QDir::currentPath());

   if ( !value.isEmpty() )
   {
      includes.append(" -I ");
      includes.append(dir.fromNativeSeparators(dir.relativeFilePath(value)));
      ui->compilerIncludePaths->setText(includes);
   }
}

void ProjectPropertiesDialog::on_assemblerIncludePathBrowse_clicked()
{
   QString value = QFileDialog::getExistingDirectory(this,"Additional Include Path",QDir::currentPath());
   QString includes = ui->assemblerIncludePaths->text();
   QDir dir(QDir::currentPath());

   if ( !value.isEmpty() )
   {
      includes.append(" -I ");
      includes.append(dir.fromNativeSeparators(dir.relativeFilePath(value)));
      ui->assemblerIncludePaths->setText(includes);
   }
}

void ProjectPropertiesDialog::on_buttonBox_accepted()
{
   nesicideProject->setProjectTitle(ui->projectNameLineEdit->text());
   nesicideProject->setProjectOutputBasePath(ui->projectOutputBasePath->text());
   nesicideProject->setProjectOutputName(ui->outputName->text());
   nesicideProject->setProjectHeaderFileName(ui->projectHeaderName->text());
   nesicideProject->setProjectSourceFileName(ui->projectSourceName->text());
   nesicideProject->setProjectLinkerOutputName(ui->linkerOutputName->text());
   nesicideProject->setProjectDebugInfoName(ui->debugInfoName->text());
   nesicideProject->setProjectCHRROMOutputBasePath(ui->chrromOutputBasePath->text());
   nesicideProject->setProjectCHRROMOutputName(ui->chrromOutputName->text());
   nesicideProject->setProjectUsesCHRROM(ui->chrRom->isChecked());
   nesicideProject->setProjectCartridgeOutputName(ui->cartridgeOutputName->text());
   nesicideProject->setProjectCartridgeSaveStateName(ui->cartridgeSaveStateName->text());
   nesicideProject->setCompilerDefinedSymbols(ui->compilerDefinedSymbols->text());
   nesicideProject->setCompilerIncludePaths(ui->compilerIncludePaths->text());
   nesicideProject->setCompilerAdditionalOptions(ui->compilerAdditionalOptions->text());
   nesicideProject->setAssemblerDefinedSymbols(ui->assemblerDefinedSymbols->text());
   nesicideProject->setAssemblerIncludePaths(ui->assemblerIncludePaths->text());
   nesicideProject->setAssemblerAdditionalOptions(ui->assemblerAdditionalOptions->text());
   nesicideProject->setProjectLinkerOutputBasePath(ui->prgromOutputBasePath->text());
   nesicideProject->setLinkerAdditionalOptions(ui->linkerAdditionalOptions->text());
   nesicideProject->setLinkerAdditionalDependencies(ui->linkerAdditionalDependencies->text());
   nesicideProject->setTileProperties(tilePropertyListModel->getItems());
   nesicideProject->setLinkerConfigFile(ui->linkerConfigFile->text());
   serializeLinkerConfig();
   nesicideProject->setMakefileCustomRulesFile(ui->customRuleFile->text());
   serializeCustomRules();

   nesicideProject->getProjectPaletteEntries()->clear();

   for (int paletteItemIndex=0; paletteItemIndex<currentPalette.count(); paletteItemIndex++)
   {
      nesicideProject->getProjectPaletteEntries()->append(currentPalette.at(paletteItemIndex));
   }

   nesicideProject->getCartridge()->setMapperNumber(mapperIDFromIndex(ui->mapperComboBox->currentIndex()));
   nesicideProject->getCartridge()->setMirrorMode((eMirrorMode)ui->mirroringComboBox->currentIndex());
   nesicideProject->getCartridge()->setFourScreen(ui->extraVRAMPresent->isChecked());
}

void ProjectPropertiesDialog::on_projectOutputBasePathBrowse_clicked()
{
   QString value = QFileDialog::getExistingDirectory(this,"Project Output Base Path",QDir::currentPath());
   QDir dir(QDir::currentPath());

   if ( dir.fromNativeSeparators(dir.relativeFilePath(value)).isEmpty() )
   {
      value = ".";
   }

   if ( !value.isEmpty() )
   {
      ui->projectOutputBasePath->setText(dir.fromNativeSeparators(dir.relativeFilePath(value)));
      ui->prgromOutputBasePath->setText(dir.fromNativeSeparators(dir.relativeFilePath(value)));
      ui->chrromOutputBasePath->setText(dir.fromNativeSeparators(dir.relativeFilePath(value)));
   }
}

void ProjectPropertiesDialog::on_projectNameLineEdit_textEdited(QString )
{
    QString text = ui->projectNameLineEdit->text();

    text = text.toLower();
    text.replace(" ","_");

    ui->outputName->setText(text);
    ui->linkerOutputName->setText(text+".prg");
    ui->debugInfoName->setText(text+".dbg");
    ui->chrromOutputName->setText(text+".chr");
    ui->cartridgeOutputName->setText(text+".nes");
    ui->cartridgeSaveStateName->setText(text+".sav");
}

void ProjectPropertiesDialog::on_linkerConfigFileBrowse_clicked()
{
   QString value = QFileDialog::getSaveFileName(this,"Linker Config File",QDir::currentPath());
   QDir dir(QDir::currentPath());

   if ( !value.isEmpty() )
   {
      ui->linkerConfigFile->setText(dir.fromNativeSeparators(dir.relativeFilePath(value)));
      ui->linkerConfig->setEnabled(true);
      ui->linkerConfig->setText("");
   }
   deserializeLinkerConfig();
}

void ProjectPropertiesDialog::serializeLinkerConfig()
{
   if ( !ui->linkerConfigFile->text().isEmpty() )
   {
      if ( linkerConfigChanged )
      {
         QDir dir(QDir::currentPath());
         QFile fileOut(dir.filePath(ui->linkerConfigFile->text()));

         fileOut.open(QIODevice::ReadWrite|QIODevice::Truncate|QIODevice::Text);
         if ( fileOut.isOpen() )
         {
            fileOut.write(ui->linkerConfig->toPlainText().toLatin1());
            fileOut.close();
         }
         else
         {
            QMessageBox::critical(0,"File I/O Error", "Could not write linker config file:\n"+ui->linkerConfigFile->text());
         }
      }

      linkerConfigChanged = false;
   }
}

void ProjectPropertiesDialog::deserializeLinkerConfig()
{
   QDir dir(QDir::currentPath());

   if ( !ui->linkerConfigFile->text().isEmpty() )
   {
      QFile fileIn(dir.filePath(ui->linkerConfigFile->text()));

      if ( fileIn.exists() )
      {
         fileIn.open(QIODevice::ReadOnly|QIODevice::Text);
         if ( fileIn.isOpen() )
         {
            ui->linkerConfig->setText(QString(fileIn.readAll()));
            fileIn.close();
         }
         else
         {
            QMessageBox::critical(0,"File I/O Error", "Could not read linker config file:\n"+ui->linkerConfigFile->text());
         }
      }

      linkerConfigChanged = false;
   }
}

void ProjectPropertiesDialog::on_linkerConfig_textChanged()
{
   // Trigger deserialization of linker config file on dialog close.
   linkerConfigChanged = true;
}

void ProjectPropertiesDialog::on_outputName_textEdited(QString )
{
   QString text = ui->outputName->text();

   ui->linkerOutputName->setText(text+".prg");
   ui->debugInfoName->setText(text+".dbg");
   ui->chrromOutputName->setText(text+".chr");
   ui->cartridgeOutputName->setText(text+".nes");
}

void ProjectPropertiesDialog::on_prgromOutputBasePathBrowse_clicked()
{
   QString value = QFileDialog::getExistingDirectory(this,"PRG Data Output Base Path",QDir::currentPath());
   QDir dir(QDir::currentPath());

   if ( dir.fromNativeSeparators(dir.relativeFilePath(value)).isEmpty() )
   {
      value = ".";
   }

   if ( !value.isEmpty() )
   {
      ui->prgromOutputBasePath->setText(dir.fromNativeSeparators(dir.relativeFilePath(value)));
   }
}

void ProjectPropertiesDialog::on_chrromOutputBasePathBrowse_clicked()
{
   QString value = QFileDialog::getExistingDirectory(this,"CHR Data Output Base Path",QDir::currentPath());
   QDir dir(QDir::currentPath());

   if ( dir.fromNativeSeparators(dir.relativeFilePath(value)).isEmpty() )
   {
      value = ".";
   }

   if ( !value.isEmpty() )
   {
      ui->chrromOutputBasePath->setText(dir.fromNativeSeparators(dir.relativeFilePath(value)));
   }
}

void ProjectPropertiesDialog::on_addProperty_clicked()
{
   PropertyEditorDialog ped;
   PropertyItem item;
   QList<PropertyItem> items;

   int result = ped.exec();

   if ( result == QDialog::Accepted )
   {
      item.name = ped.propertyName();
      item.type = ped.propertyType();
      item.value = ped.propertyValue();
      if ( !item.name.isEmpty() )
      {
         items = tilePropertyListModel->getItems();
         items.append(item);
         tilePropertyListModel->setItems(items);
         tilePropertyListModel->update();
      }
   }
}

void ProjectPropertiesDialog::on_removeProperty_clicked()
{
   QModelIndex index = ui->propertyTableView->currentIndex();
   QList<PropertyItem> items = tilePropertyListModel->getItems();

   if ( index.isValid() )
   {
      items.removeAt(index.row());
      tilePropertyListModel->setItems(items);
      tilePropertyListModel->update();
   }
}

void ProjectPropertiesDialog::on_propertyTableView_doubleClicked(QModelIndex index)
{
   PropertyEditorDialog ped;
   QList<PropertyItem> items = tilePropertyListModel->getItems();
   PropertyItem item;
   int result;

   if ( index.isValid() )
   {
      ped.setPropertyName(items.at(index.row()).name);
      ped.setPropertyType(items.at(index.row()).type);
      ped.setPropertyValue(items.at(index.row()).value);
      result = ped.exec();
      if ( result == QDialog::Accepted )
      {
         item.name = ped.propertyName();
         item.type = ped.propertyType();
         item.value = ped.propertyValue();
         items = tilePropertyListModel->getItems();
         items.replace(index.row(),item);
         tilePropertyListModel->setItems(items);
         tilePropertyListModel->update();
      }
   }
}

void ProjectPropertiesDialog::on_editProperty_clicked()
{
   on_propertyTableView_doubleClicked(ui->propertyTableView->currentIndex());
}

void ProjectPropertiesDialog::on_treeWidget_itemSelectionChanged()
{
   ui->treeWidget->expand(ui->treeWidget->currentIndex());
   if ( ui->treeWidget->currentItem()->childCount() )
   {
      ui->treeWidget->setCurrentItem(ui->treeWidget->itemBelow(ui->treeWidget->currentItem()));
   }
   ui->stackedWidget->setCurrentWidget(pageMap[ui->treeWidget->currentItem()->text(0)]);
}

void ProjectPropertiesDialog::serializeCustomRules()
{
   if ( !ui->customRuleFile->text().isEmpty() )
   {
      if ( customRulesChanged )
      {
         QDir dir(QDir::currentPath());
         QFile fileOut(dir.filePath(ui->customRuleFile->text()));

         fileOut.open(QIODevice::ReadWrite|QIODevice::Truncate|QIODevice::Text);
         if ( fileOut.isOpen() )
         {
            fileOut.write(ui->customRules->toPlainText().toLatin1());
            fileOut.close();
         }
         else
         {
            QMessageBox::critical(0,"File I/O Error", "Could not write custom rules file:\n"+ui->customRuleFile->text());
         }
      }

      customRulesChanged = false;
   }
}

void ProjectPropertiesDialog::deserializeCustomRules()
{
   QDir dir(QDir::currentPath());

   if ( !ui->customRuleFile->text().isEmpty() )
   {
      QFile fileIn(dir.filePath(ui->customRuleFile->text()));

      if ( fileIn.exists() )
      {
         fileIn.open(QIODevice::ReadOnly|QIODevice::Text);
         if ( fileIn.isOpen() )
         {
            ui->customRules->setText(QString(fileIn.readAll()));
            fileIn.close();
         }
         else
         {
            QMessageBox::critical(0,"File I/O Error", "Could not read custom rules file:\n"+ui->customRuleFile->text());
         }
      }

      customRulesChanged = false;
   }
}

void ProjectPropertiesDialog::on_customRules_textChanged()
{
   // Trigger deserialization of custom rule file on dialog close.
    customRulesChanged = true;
}

void ProjectPropertiesDialog::on_customRuleFileBrowse_clicked()
{
   QString value = QFileDialog::getSaveFileName(this,"Custom Rule File",QDir::currentPath());
   QDir dir(QDir::currentPath());

   if ( !value.isEmpty() )
   {
      ui->customRuleFile->setText(dir.fromNativeSeparators(dir.relativeFilePath(value)));
      ui->customRules->setEnabled(true);
      ui->customRules->setText("");
   }
   deserializeCustomRules();
}
