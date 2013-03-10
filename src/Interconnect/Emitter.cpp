/*
    This file is part of Corrade.

    Copyright © 2007, 2008, 2009, 2010, 2011, 2012, 2013
              Vladimír Vondruš <mosra@centrum.cz>

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

#include "Emitter.h"

#include "Utility/Assert.h"
#include "Receiver.h"

namespace Corrade { namespace Interconnect {

Emitter::~Emitter() {
    for(auto it = connections.begin(); it != connections.end(); ++it) {
        /* Remove connection from receiver */
        for(auto rit = it->second->receiver->connections.begin(); rit != it->second->receiver->connections.end(); ++rit) {
            if(*rit != it->second) continue;

            it->second->receiver->connections.erase(rit);
            break;
        }

        /* If there is connection object, remove reference to connection data
           from it and mark it as disconnected */
        if(it->second->connection) {
            CORRADE_INTERNAL_ASSERT(it->second == it->second->connection->data);
            it->second->connection->data = nullptr;
            it->second->connection->connected = false;
        }

        /* Delete connection data (as they make no sense without emitter) */
        delete it->second;
    }
}

void Emitter::connect(const Implementation::SignalData& signal, Implementation::AbstractConnectionData* data) {
    /* Add connection to emitter */
    data->emitter->connections.insert(std::make_pair(signal, data));
    data->emitter->connectionsChanged = true;

    /* Add connection to receiver */
    data->receiver->connections.push_back(data);

    /* If there is connection object, mark the connection as connected */
    if(data->connection) data->connection->connected = true;
}

void Emitter::disconnect(const Implementation::SignalData& signal, Implementation::AbstractConnectionData* data) {
    /* Find given connection, disconnect it and erase */
    auto range = data->emitter->connections.equal_range(signal);
    for(auto it = range.first; it != range.second; ++it) {
        if(it->second != data) continue;

        data->emitter->disconnect(it);
        data->emitter->connections.erase(it);
        data->emitter->connectionsChanged = true;
        return;
    }

    /* The connection must be found */
    CORRADE_INTERNAL_ASSERT(false);
}

void Emitter::disconnect(const Implementation::SignalData& signal) {
    auto range = connections.equal_range(signal);
    for(auto it = range.first; it != range.second; ++it)
        disconnect(it);

    connections.erase(range.first, range.second);
    connectionsChanged = true;
}

void Emitter::disconnectAllSignals() {
    for(auto it = connections.begin(); it != connections.end(); ++it)
        disconnect(it);

    connections.clear();
    connectionsChanged = true;
}

void Emitter::disconnect(std::unordered_multimap<Implementation::SignalData, Implementation::AbstractConnectionData*, Implementation::SignalDataHash>::const_iterator it) {
    Implementation::AbstractConnectionData* data = it->second;

    /* Remove connection from receiver */
    for(auto end = data->receiver->connections.end(), rit = data->receiver->connections.begin(); rit != end; ++rit) {
        if(*rit != data) continue;

        data->receiver->connections.erase(rit);
        break;
    }

    /* If there is no connection object, destroy also connection data (as we
       are the last remaining owner) */
    if(!data->connection) delete data;

    /* Else mark the connection as disconnected */
    else data->connection->connected = false;

    /* (erasing the iterator is up to the caller) */
}

}}
