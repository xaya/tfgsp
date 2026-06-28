// Trimmed vendored copy of libxayagame's xayagame/testutils.{hpp,cpp}
// (MIT licence, Copyright (C) 2018-2026 The Xaya developers).
//
// Only the pieces needed to drive a real xaya::Game + SQLiteGame through the
// production BlockAttach / BlockDetach (reorg) path are kept: BlockHash, the
// GameTestFixture (which Game declares a friend, so it may reach Game's private
// block-processing entry points) and GameTestWithBlockchain (the attach/detach
// stack).  The upstream MockXayaRpcServer / HttpRpcServer machinery is dropped
// because the SQLiteGame reorg path is exercised with a null RPC client and
// needs no live server -- which also keeps this free of the gmock / jsonrpc
// server dependencies.
//
// One deliberate divergence from upstream: CallBlockAttach / CallBlockDetach
// add a block "timestamp", which PXLogic::UpdateState requires (logic.cpp) but
// the upstream helper omits.  Keep this file in sync with upstream by hand.

#ifndef PXD_XGAMETESTUTILS_HPP
#define PXD_XGAMETESTUTILS_HPP

#include <xayagame/game.hpp>

#include <xayautil/uint256.hpp>

#include <json/json.h>

#include <gtest/gtest.h>

#include <mutex>
#include <string>
#include <vector>

namespace xaya
{

/**
 * Returns a uint256 based on the given number, to be used as block hashes
 * in tests.
 */
uint256 BlockHash (unsigned num);

/**
 * Test fixture that is a friend of Game, exposing the internal block-processing
 * entry points (and a couple of state helpers) to tests.
 */
class GameTestFixture : public testing::Test
{

private:

  /** Game ID to send to BlockAttach / BlockDetach.  */
  const std::string gameId;

protected:

  using State = Game::State;

  explicit GameTestFixture (const std::string& id)
    : gameId(id)
  {}

  static State
  GetState (const Game& g)
  {
    return g.state;
  }

  static void
  ForceState (Game& g, const State s)
  {
    std::lock_guard<std::mutex> lock(g.mut);
    g.state = s;
  }

  /**
   * Calls BlockAttach on the given Game instance, building the blockData JSON
   * (including a timestamp, which PXLogic requires) from the pieces given here.
   */
  void CallBlockAttach (Game& g, const std::string& reqToken,
                        const uint256& parentHash, const uint256& blockHash,
                        unsigned height,
                        const Json::Value& moves, bool seqMismatch) const;

  /**
   * Calls BlockDetach on the given Game instance, building the blockData object
   * correctly.
   */
  void CallBlockDetach (Game& g, const std::string& reqToken,
                        const uint256& parentHash, const uint256& blockHash,
                        unsigned height,
                        const Json::Value& moves, bool seqMismatch) const;

};

/**
 * An extension to GameTestFixture that keeps track of its own "blockchain" in a
 * stack of block hashes and move objects, so attaches/detaches are consistent.
 */
class GameTestWithBlockchain : public GameTestFixture
{

private:

  /** Height of the first (starting) block in blockHashes.  */
  unsigned heightOffset;

  /** Stack of attached block hashes.  */
  std::vector<uint256> blockHashes;

  /** Move objects matching blockHashes (the starting block has none).  */
  std::vector<Json::Value> moveStack;

public:

  using GameTestFixture::GameTestFixture;

  /** Resets the simulated chain to the given starting block.  */
  void SetStartingBlock (unsigned height, const uint256& hash);

  /** Attaches the given next block on top of the current stack.  */
  void AttachBlock (Game& g, const uint256& hash, const Json::Value& moves);

  /** Detaches the current top block from the simulated chain.  */
  void DetachBlock (Game& g);

};

} // namespace xaya

#endif // PXD_XGAMETESTUTILS_HPP
