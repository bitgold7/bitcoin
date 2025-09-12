#include <qt/dividendpage.h>

#include <QStringList>
#include <interfaces/node.h>
#include <qt/bitcoinunits.h>
#include <qt/clientmodel.h>
#include <qt/optionsmodel.h>
#include <qt/walletmodel.h>
#include <wallet/wallet.h>

#include <QLabel>
#include <QVBoxLayout>

DividendPage::DividendPage(const PlatformStyle* platformStyle, QWidget* parent)
    : QWidget(parent), m_platform_style(platformStyle)
{
    QVBoxLayout* vbox = new QVBoxLayout(this);
    poolLabel = new QLabel(tr("Pool: 0"), this);
    nextLabel = new QLabel(tr("Next height: n/a"), this);
    payoutsLabel = new QLabel(tr("Payouts: n/a"), this);
    vbox->addWidget(poolLabel);
    vbox->addWidget(nextLabel);
    vbox->addWidget(payoutsLabel);
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
    QStringList payouts;
    for (const auto& [addr, amt] : next.second) {
        payouts << QString::fromStdString(addr) + ": " + BitcoinUnits::format(walletModel->getOptionsModel()->getDisplayUnit(), amt);
    }
    payoutsLabel->setText(tr("Payouts: %1").arg(payouts.join(", ")));
}
