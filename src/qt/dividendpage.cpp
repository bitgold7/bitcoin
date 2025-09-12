#include <qt/dividendpage.h>

#include <qt/bitcoinunits.h>
#include <qt/walletmodel.h>
#include <qt/clientmodel.h>
#include <qt/optionsmodel.h>
#include <interfaces/node.h>
#include <wallet/wallet.h>

#include <QVBoxLayout>
#include <QLabel>

DividendPage::DividendPage(const PlatformStyle* platformStyle, QWidget* parent)
    : QWidget(parent), m_platform_style(platformStyle)
{
    QVBoxLayout* vbox = new QVBoxLayout(this);
    poolLabel = new QLabel(tr("Pool: 0"), this);
    nextLabel = new QLabel(tr("Next height: n/a"), this);
    vbox->addWidget(poolLabel);
    vbox->addWidget(nextLabel);
}

void DividendPage::setClientModel(ClientModel* model)
{
    clientModel = model;
}

void DividendPage::setWalletModel(WalletModel* model)
{
    walletModel = model;
}

void DividendPage::updateData()
{
    if (!walletModel) return;
    CAmount pool = walletModel->wallet().getDividendBalance();
    poolLabel->setText(tr("Pool: %1").arg(BitcoinUnits::format(walletModel->getOptionsModel()->getDisplayUnit(), pool)));
    auto next = walletModel->wallet().getNextDividend();
    nextLabel->setText(tr("Next height: %1").arg(next.first));
}
