/********************************************************************************
** Form generated from reading UI file 'manager.ui'
**
** Created by: Qt User Interface Compiler version 5.6.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QDialog>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_Manager
{
public:
    QHBoxLayout *horizontalLayout;
    QVBoxLayout *verticalLayout;
    QLabel *label;
    QLineEdit *editName;
    QLabel *label_2;
    QHBoxLayout *horizontalLayout_2;
    QLineEdit *editPath;
    QPushButton *btnBrowse;
    QPushButton *btnSave;

    void setupUi(QDialog *Manager)
    {
        if (Manager->objectName().isEmpty())
            Manager->setObjectName(QStringLiteral("Manager"));
        Manager->resize(292, 156);
        QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(Manager->sizePolicy().hasHeightForWidth());
        Manager->setSizePolicy(sizePolicy);
        Manager->setToolTipDuration(0);
        Manager->setModal(false);
        horizontalLayout = new QHBoxLayout(Manager);
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        verticalLayout = new QVBoxLayout();
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        label = new QLabel(Manager);
        label->setObjectName(QStringLiteral("label"));

        verticalLayout->addWidget(label);

        editName = new QLineEdit(Manager);
        editName->setObjectName(QStringLiteral("editName"));

        verticalLayout->addWidget(editName);

        label_2 = new QLabel(Manager);
        label_2->setObjectName(QStringLiteral("label_2"));

        verticalLayout->addWidget(label_2);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QStringLiteral("horizontalLayout_2"));
        editPath = new QLineEdit(Manager);
        editPath->setObjectName(QStringLiteral("editPath"));
        editPath->setReadOnly(true);

        horizontalLayout_2->addWidget(editPath);

        btnBrowse = new QPushButton(Manager);
        btnBrowse->setObjectName(QStringLiteral("btnBrowse"));

        horizontalLayout_2->addWidget(btnBrowse);


        verticalLayout->addLayout(horizontalLayout_2);

        btnSave = new QPushButton(Manager);
        btnSave->setObjectName(QStringLiteral("btnSave"));

        verticalLayout->addWidget(btnSave);


        horizontalLayout->addLayout(verticalLayout);


        retranslateUi(Manager);

        QMetaObject::connectSlotsByName(Manager);
    } // setupUi

    void retranslateUi(QDialog *Manager)
    {
        Manager->setWindowTitle(QApplication::translate("Manager", "Manager", 0));
        label->setText(QApplication::translate("Manager", "Client Name", 0));
        label_2->setText(QApplication::translate("Manager", "Client Path", 0));
        btnBrowse->setText(QApplication::translate("Manager", "Browse", 0));
        btnSave->setText(QApplication::translate("Manager", "Save", 0));
    } // retranslateUi

};

namespace Ui {
    class Manager: public Ui_Manager {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MANAGER_H
