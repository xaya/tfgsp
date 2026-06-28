// Trimmed vendored copy of libxayagame's xayagame/testutils.cpp
// (MIT licence, Copyright (C) 2018-2026 The Xaya developers).  See
// xgametestutils.hpp for the rationale and the one deliberate divergence
// (a block timestamp, which PXLogic::UpdateState requires).

#include "xgametestutils.hpp"

#include <glog/logging.h>

#include <cstdio>

namespace xaya
{

uint256
BlockHash (unsigned num)
{
  std::string hex = "ab" + std::string (62, '0');

  CHECK (num < 0x100);
  std::sprintf (&hex[2], "%02x", num);
  CHECK (hex[4] == 0);
  hex[4] = '0';

  uint256 res;
  CHECK (res.FromHex (hex));
  return res;
}

namespace
{

/**
 * Builds the blockData JSON for a CallBlockAttach / CallBlockDetach.  Adds the
 * "timestamp" field that PXLogic::UpdateState requires (upstream omits it);
 * fixed per height so re-attaching the same block reproduces it exactly.
 */
Json::Value
BuildBlockData (const std::string& reqToken, const uint256& parentHash,
                const uint256& blockHash, const unsigned height,
                const Json::Value& moves)
{
  Json::Value block(Json::objectValue);
  block["hash"] = blockHash.ToHex ();
  block["parent"] = parentHash.ToHex ();
  block["height"] = height;
  block["rngseed"] = blockHash.ToHex ();
  block["timestamp"] = static_cast<Json::Int64> (1500000000 + height);

  Json::Value data(Json::objectValue);
  if (!reqToken.empty ())
    data["reqtoken"] = reqToken;
  data["block"] = block;
  data["moves"] = moves;
  /* PXLogic::UpdateState also requires an "admin" array (the real Xaya feed
     supplies one); the upstream helper omits it.  Empty is fine here.  */
  data["admin"] = Json::Value (Json::arrayValue);

  return data;
}

} // anonymous namespace

void
GameTestFixture::CallBlockAttach (Game& g, const std::string& reqToken,
                                  const uint256& parentHash,
                                  const uint256& blockHash,
                                  const unsigned height,
                                  const Json::Value& moves,
                                  const bool seqMismatch) const
{
  g.BlockAttach (gameId,
                 BuildBlockData (reqToken, parentHash, blockHash, height, moves),
                 seqMismatch);
}

void
GameTestFixture::CallBlockDetach (Game& g, const std::string& reqToken,
                                  const uint256& parentHash,
                                  const uint256& blockHash,
                                  const unsigned height,
                                  const Json::Value& moves,
                                  const bool seqMismatch) const
{
  g.BlockDetach (gameId,
                 BuildBlockData (reqToken, parentHash, blockHash, height, moves),
                 seqMismatch);
}

void
GameTestWithBlockchain::SetStartingBlock (const unsigned height,
                                          const uint256& hash)
{
  heightOffset = height;
  blockHashes = {hash};
  moveStack.clear ();
}

void
GameTestWithBlockchain::AttachBlock (Game& g, const uint256& hash,
                                     const Json::Value& moves)
{
  CHECK (!blockHashes.empty ()) << "No starting block has been set";
  CallBlockAttach (g, "",
                   blockHashes.back (), hash,
                   heightOffset + blockHashes.size (),
                   moves, false);
  blockHashes.push_back (hash);
  moveStack.push_back (moves);
}

void
GameTestWithBlockchain::DetachBlock (Game& g)
{
  CHECK (!blockHashes.empty ());
  CHECK (!moveStack.empty ());

  const uint256 hash = blockHashes.back ();
  blockHashes.pop_back ();
  CallBlockDetach (g, "",
                   blockHashes.back (), hash,
                   heightOffset + blockHashes.size (),
                   moveStack.back (), false);
  moveStack.pop_back ();
}

} // namespace xaya
