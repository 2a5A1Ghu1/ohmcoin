// Copyright (c) 2019 The Ohmcoin Developers
// Copyright (c) 2018 The Curium developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include "config/ohmcoin-config.h"
#endif

#include "configurekarmanodepage.h"
#include "ui_configurekarmanodepage.h"

#include "activekarmanode.h"
#include "bitcoingui.h"
#include "csvmodelwriter.h"
#include "editaddressdialog.h"
#include "guiutil.h"
#include "karmanode-budget.h"
#include "karmanode-payments.h"
#include "karmanodeconfig.h"
#include "karmanodeman.h"
#include "karmanodelist.h"
#include "wallet/wallet.h"

#include <univalue.h>
#include <QIcon>
#include <QMenu>
#include <QString>
#include <QMessageBox>
#include <QSortFilterProxyModel>
#include <boost/tokenizer.hpp>
#include <fstream>
#include <iostream>
#include <string>

ConfigureKarmanodePage::ConfigureKarmanodePage(Mode mode, QWidget* parent) : QDialog(parent),
                                                                   ui(new Ui::ConfigureKarmanodePage),
                                                                   mapper(0),
                                                                   mode(mode)
{
    ui->setupUi(this);

	GUIUtil::setupAliasWidget(ui->aliasEdit, this);
	GUIUtil::setupIPWidget(ui->vpsIpEdit, this);
	GUIUtil::setupPrivKeyWidget(ui->privKeyEdit, this);
	GUIUtil::setupTXIDWidget(ui->outputEdit, this);
	GUIUtil::setupTXIDIndexWidget(ui->outputIdEdit, this);

    switch (mode) {
    case NewConfigureKarmanode:
        setWindowTitle(tr("New Karmanode Alias"));
        break;
    case EditConfigureKarmanode:
        setWindowTitle(tr("Edit Karmanode Alias"));
        break;
    }

}

ConfigureKarmanodePage::~ConfigureKarmanodePage()
{
    delete ui;
}


void ConfigureKarmanodePage::loadAlias(QString strAlias)
{
   ui->aliasEdit->setText(strAlias);
}

void ConfigureKarmanodePage::counter(int counter)
{
   setCounters(counter);
}


void ConfigureKarmanodePage::MNAliasCache(QString MnAliasCache)
{
   setMnAliasCache(MnAliasCache);
}

void ConfigureKarmanodePage::loadIP(QString strIP)
{
   ui->vpsIpEdit->setText(strIP);
}

void ConfigureKarmanodePage::loadPrivKey(QString strPrivKey)
{
   ui->privKeyEdit->setText(strPrivKey);
}

void ConfigureKarmanodePage::loadTxHash(QString strTxHash)
{
   ui->outputEdit->setText(strTxHash);
}

void ConfigureKarmanodePage::loadOutputIndex(QString strOutputIndex)
{
   ui->outputIdEdit->setText(strOutputIndex);
}


void ConfigureKarmanodePage::saveCurrentRow()
{

    switch (mode) {
    case NewConfigureKarmanode:
		if(ui->aliasEdit->text().toStdString().empty() || ui->vpsIpEdit->text().toStdString().empty() || ui->privKeyEdit->text().toStdString().empty() || ui->outputEdit->text().toStdString().empty() || ui->outputIdEdit->text().toStdString().empty()) {
			break;
		}
		karmanodeConfig.add(ui->aliasEdit->text().toStdString(), ui->vpsIpEdit->text().toStdString(), ui->privKeyEdit->text().toStdString(), ui->outputEdit->text().toStdString(), ui->outputIdEdit->text().toStdString());
		karmanodeConfig.writeToKarmanodeConf();
        break;
    case EditConfigureKarmanode:
		if(ui->aliasEdit->text().toStdString().empty() || ui->vpsIpEdit->text().toStdString().empty() || ui->privKeyEdit->text().toStdString().empty() || ui->outputEdit->text().toStdString().empty() || ui->outputIdEdit->text().toStdString().empty()) {
			break;
		}

	    QString MnAlias = getMnAliasCache();
		ConfigureKarmanodePage::updateAlias(ui->aliasEdit->text().toStdString(), ui->vpsIpEdit->text().toStdString(), ui->privKeyEdit->text().toStdString(), ui->outputEdit->text().toStdString(), ui->outputIdEdit->text().toStdString(), MnAlias.toStdString());
		break;
    }
}

void ConfigureKarmanodePage::accept()
{
	saveCurrentRow();
	emit accepted();
    QDialog::accept();
}


void ConfigureKarmanodePage::updateAlias(std::string Alias, std::string IP, std::string PrivKey, std::string TxHash, std::string OutputIndex, std::string mnAlias)
{
	for (CKarmanodeConfig::CKarmanodeEntry mne : karmanodeConfig.getEntries()) {
		if(mnAlias == mne.getAlias()) {
			int count = 0;
			count = getCounters();
			vector<COutPoint> confLockedCoins;
			uint256 mnTxHash;
			mnTxHash.SetHex(mne.getTxHash());
			int nIndex;
			if(!mne.castOutputIndex(nIndex))
				continue;
			COutPoint outpoint = COutPoint(mnTxHash, nIndex);
			confLockedCoins.push_back(outpoint);
			pwalletMain->UnlockCoin(outpoint);

			karmanodeConfig.deleteAlias(count);
			karmanodeConfig.add(Alias, IP, PrivKey, TxHash, OutputIndex);
			// write to karmanode.conf
			karmanodeConfig.writeToKarmanodeConf();
			return;
		}
	}

}

void ConfigureKarmanodePage::on_AutoFillPrivKey_clicked()
{
    CKey secret;
    secret.MakeNewKey(false);

	ui->privKeyEdit->setText(QString::fromStdString(CBitcoinSecret(secret).ToString()));
}


void ConfigureKarmanodePage::on_AutoFillOutputs_clicked()
{
    // Find possible candidates
    vector<COutput> possibleCoins = activeKarmanode.SelectCoinsKarmanode();
        int test = 0;
    for (COutput& out : possibleCoins) {
        std::string TXHash = out.tx->GetHash().ToString();
        std::string OutputID = std::to_string(out.i);
                for (CKarmanodeConfig::CKarmanodeEntry mne : karmanodeConfig.getEntries()) {
                        if(OutputID == mne.getOutputIndex() && TXHash == mne.getTxHash()) {
                                test = 1;

                        }
                }

                if(test == 0) {
                        ui->outputEdit->setText(QString::fromStdString(out.tx->GetHash().ToString()));
                        ui->outputIdEdit->setText(QString::fromStdString(std::to_string(out.i)));

                        break;
                }
                test = 0;
    }
}

