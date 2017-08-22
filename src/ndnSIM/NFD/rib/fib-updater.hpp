/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2016,  Regents of the University of California,
 *                           Arizona Board of Regents,
 *                           Colorado State University,
 *                           University Pierre & Marie Curie, Sorbonne University,
 *                           Washington University in St. Louis,
 *                           Beijing Institute of Technology,
 *                           The University of Memphis.
 *
 * This file is part of NFD (Named Data Networking Forwarding Daemon).
 * See AUTHORS.md for complete list of NFD authors and contributors.
 *
 * NFD is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * NFD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * NFD, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef NFD_RIB_FIB_UPDATER_HPP
#define NFD_RIB_FIB_UPDATER_HPP

#include "core/common.hpp"
#include "fib-update.hpp"
#include "rib.hpp"
#include "rib-update-batch.hpp"

#include <ndn-cxx/mgmt/nfd/controller.hpp>

namespace nfd {
    namespace rib {

        /** \brief computes FibUpdates based on updates to the RIB and sends them to NFD
         */
        class FibUpdater : noncopyable {
        public:

            class Error : public std::runtime_error {
            public:

                explicit
                Error(const std::string& what)
                : std::runtime_error(what) {
                }
            };

        public:
            typedef std::list<FibUpdate> FibUpdateList;

            typedef function<void(RibUpdateList inheritedRoutes) > FibUpdateSuccessCallback;
            typedef function<void(uint32_t code, const std::string& error) > FibUpdateFailureCallback;

            FibUpdater(Rib& rib, ndn::nfd::Controller& controller);

            /** \brief computes FibUpdates using the provided RibUpdateBatch and then sends the
             *         updates to NFD's FIB
             *
             *  \note  Caller must guarantee that the previous batch has either succeeded or failed
             *         before calling this method
             */
            void
            computeAndSendFibUpdates(const RibUpdateBatch& batch,
                    const FibUpdateSuccessCallback& onSuccess,
                    const FibUpdateFailureCallback& onFailure);

PUBLIC_WITH_TESTS_ELSE_PRIVATE:
            /** \brief determines the type of action that will be performed on the RIB and calls the
             *          corresponding computation method
             */
            void
            computeUpdates(const RibUpdateBatch& batch);

            /** \brief sends the passed updates to NFD
             *
             *   onSuccess or onFailure will be called based on the results in
             *   onUpdateSuccess or onUpdateFailure
             *
             *   \see FibUpdater::onUpdateSuccess
             *   \see FibUpdater::onUpdateFailure
             */
            void
            sendUpdates(const FibUpdateList& updates,
                    const FibUpdateSuccessCallback& onSuccess,
                    const FibUpdateFailureCallback& onFailure);

            /** \brief sends the updates in m_updatesForBatchFaceId to NFD if any exist,
             *          otherwise calls FibUpdater::sendUpdatesForNonBatchFaceId.
             */
            void
            sendUpdatesForBatchFaceId(const FibUpdateSuccessCallback& onSuccess,
                    const FibUpdateFailureCallback& onFailure);

            /** \brief sends the updates in m_updatesForNonBatchFaceId to NFD if any exist,
             *          otherwise calls onSuccess.
             */
            void
            sendUpdatesForNonBatchFaceId(const FibUpdateSuccessCallback& onSuccess,
                    const FibUpdateFailureCallback& onFailure);

            /** \brief sends a FibAddNextHopCommand to NFD using the parameters supplied by
             *          the passed update
             *
             *   \param nTimeouts the number of times this FibUpdate has failed due to timeout
             */
            void
            sendAddNextHopUpdate(const FibUpdate& update,
                    const FibUpdateSuccessCallback& onSuccess,
                    const FibUpdateFailureCallback& onFailure,
                    uint32_t nTimeouts = 0);

            /** \brief sends a FibRemoveNextHopCommand to NFD using the parameters supplied by
             *          the passed update
             *
             *   \param nTimeouts the number of times this FibUpdate has failed due to timeout
             */
            void
            sendRemoveNextHopUpdate(const FibUpdate& update,
                    const FibUpdateSuccessCallback& onSuccess,
                    const FibUpdateFailureCallback& onFailure,
                    uint32_t nTimeouts = 0);

        private:
            /** \brief calculates the FibUpdates generated by a RIB registration
             */
            void
            computeUpdatesForRegistration(const RibUpdate& update);

            /** \brief calculates the FibUpdates generated by a RIB unregistration
             */
            void
            computeUpdatesForUnregistration(const RibUpdate& update);

PUBLIC_WITH_TESTS_ELSE_PRIVATE:
            /** \brief callback used by NfdController when a FibAddNextHopCommand or FibRemoveNextHopCommand
             *          is successful.
             *
             *   If the update has the same Face ID as the batch being processed, the update is
             *   removed from m_updatesForBatchFaceId. If m_updatesForBatchFaceId becomes empty,
             *   the updates with a different Face ID than the batch are sent to NFD.
             *
             *   If the update has a different Face ID than the batch being processed, the update is
             *   removed from m_updatesForBatchNonFaceId. If m_updatesForBatchNonFaceId becomes empty,
             *   the FIB update process is considered a success.
             */
            void
            onUpdateSuccess(const FibUpdate update,
                    const FibUpdateSuccessCallback& onSuccess,
                    const FibUpdateFailureCallback& onFailure);

            /** \brief callback used by NfdController when a FibAddNextHopCommand or FibRemoveNextHopCommand
             *          is successful.
             *
             *   If the update has not reached the max number of timeouts allowed, the update
             *   is retried.
             *
             *   If the update failed due to a non-existent face and the update has the same Face ID
             *   as the update batch, the FIB update process fails.
             *
             *   If the update failed due to a non-existent face and the update has a different
             *   face than the update batch, the update is not retried and the error is
             *   ignored.
             *
             *   Otherwise, a non-recoverable error has occurred and an exception is thrown.
             */
            void
            onUpdateError(const FibUpdate update,
                    const FibUpdateSuccessCallback& onSuccess,
                    const FibUpdateFailureCallback& onFailure,
                    const ndn::nfd::ControlResponse& response, uint32_t nTimeouts);

        private:
            /** \brief adds the update to an update list based on its Face ID
             *
             *   If the update has the same Face ID as the update batch, the update is added
             *   to m_updatesForBatchFaceId.
             *
             *   Otherwise, the update is added to m_updatesForBatchNonFaceId.
             */
            void
            addFibUpdate(const FibUpdate update);

            /** \brief creates records of the passed routes added to the entry and creates FIB updates
             */
            void
            addInheritedRoutes(const RibEntry& entry, const Rib::RouteSet& routesToAdd);

            /** \brief creates records of the passed routes added to the name and creates FIB updates.
             *          Routes in routesToAdd with the same Face ID as ignore will be not be considered.
             */
            void
            addInheritedRoutes(const Name& name, const Rib::RouteSet& routesToAdd, const Route& ignore);

            /** \brief creates records of the passed routes removed from the entry and creates FIB updates
             */
            void
            removeInheritedRoutes(const RibEntry& entry, const Rib::RouteSet& routesToRemove);

            /** \brief calculates updates for a name that will create a new RIB entry
             */
            void
            createFibUpdatesForNewRibEntry(const Name& name, const Route& route,
                    const Rib::RibEntryList& children);

            /** \brief calculates updates for a new route added to a RIB entry
             */
            void
            createFibUpdatesForNewRoute(const RibEntry& entry, const Route& route,
                    const bool captureWasTurnedOn);

            /** \brief calculates updates for changes to an existing route for a RIB entry
             */
            void
            createFibUpdatesForUpdatedRoute(const RibEntry& entry, const Route& route,
                    const Route& existingRoute);

            /** \brief calculates updates for a an existing route removed from a RIB entry
             */
            void
            createFibUpdatesForErasedRoute(const RibEntry& entry, const Route& route,
                    const bool captureWasTurnedOff);

            /** \brief calculates updates for an entry that will be removed from the RIB
             */
            void
            createFibUpdatesForErasedRibEntry(const RibEntry& entry);

            /** \brief adds and removes passed routes to children's inherited routes
             */
            void
            modifyChildrensInheritedRoutes(const Rib::RibEntryList& children,
                    const Rib::RouteSet& routesToAdd,
                    const Rib::RouteSet& routesToRemove);

            /** \brief traverses the entry's children adding and removing the passed routes
             */
            void
            traverseSubTree(const RibEntry& entry, Rib::RouteSet routesToAdd, Rib::RouteSet routesToRemove);

        private:
            /** \brief creates a record of a calculated inherited route that should be added to the entry
             */
            void
            addInheritedRoute(const Name& name, const Route& route);

            /** \brief creates a record of an existing inherited route that should be removed from the entry
             */
            void
            removeInheritedRoute(const Name& name, const Route& route);

        private:
            const Rib& m_rib;
            ndn::nfd::Controller& m_controller;
            uint64_t m_batchFaceId;

PUBLIC_WITH_TESTS_ELSE_PRIVATE:
            FibUpdateList m_updatesForBatchFaceId;
            FibUpdateList m_updatesForNonBatchFaceId;

            /** \brief list of inherited routes generated during FIB update calculation;
             *         passed to the RIB when updates are completed successfully
             */
            RibUpdateList m_inheritedRoutes;

        private:
            static const unsigned int MAX_NUM_TIMEOUTS;
            static const uint32_t ERROR_FACE_NOT_FOUND;
        };

    } // namespace rib
} // namespace nfd

#endif // NFD_RIB_FIB_UPDATER_HPP
