// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOCALPLEX_PLAYER_MPV_NODE_CONVERTER_H
#define LOCALPLEX_PLAYER_MPV_NODE_CONVERTER_H

#include <mpv/client.h>
#include <QtCore/QVariant>
#include <QtCore/QVariantList>
#include <QtCore/QVariantMap>
#include <QtCore/QString>

namespace localplex::player {

inline QVariant mpvNodeToVariant(const mpv_node *node) {
    if (!node) {
        return QVariant();
    }

    switch (node->format) {
    case MPV_FORMAT_STRING:
        return QVariant(QString::fromUtf8(node->u.string));
    case MPV_FORMAT_FLAG:
        return QVariant(static_cast<bool>(node->u.flag != 0));
    case MPV_FORMAT_INT64:
        return QVariant(static_cast<qlonglong>(node->u.int64));
    case MPV_FORMAT_DOUBLE:
        return QVariant(node->u.double_);
    case MPV_FORMAT_NODE_ARRAY: {
        QVariantList list;
        if (node->u.list) {
            list.reserve(node->u.list->num);
            for (int i = 0; i < node->u.list->num; ++i) {
                list.append(mpvNodeToVariant(&node->u.list->values[i]));
            }
        }
        return list;
    }
    case MPV_FORMAT_NODE_MAP: {
        QVariantMap map;
        if (node->u.list) {
            for (int i = 0; i < node->u.list->num; ++i) {
                const char *key = node->u.list->keys[i];
                if (key) {
                    map.insert(QString::fromUtf8(key), mpvNodeToVariant(&node->u.list->values[i]));
                }
            }
        }
        return map;
    }
    case MPV_FORMAT_BYTE_ARRAY:
        if (node->u.ba) {
            return QByteArray(static_cast<const char *>(node->u.ba->data), static_cast<int>(node->u.ba->size));
        }
        return QByteArray();
    default:
        return QVariant();
    }
}

} // namespace localplex::player

#endif // LOCALPLEX_PLAYER_MPV_NODE_CONVERTER_H
