// Copyright (c) 2012-2017, The CryptoNote developers, The Bytecoin developers
//
// This file is part of Bytecoin.
//
// Bytecoin is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Bytecoin is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Bytecoin.  If not, see <http://www.gnu.org/licenses/>.

#include "TransactionUtils.h"

#include <unordered_set>

#include "crypto/crypto.h"
#include "CryptoNoteCore/Account.h"
#include "CryptoNoteFormatUtils.h"
#include "TransactionExtra.h"

using namespace Crypto;

namespace CryptoNote {

bool checkInputsKeyimagesDiff(const CryptoNote::TransactionPrefix& tx) {
  std::unordered_set<Crypto::KeyImage> ki;
  for (const auto& in : tx.inputs) {
    if (in.is<KeyInput>()) {
      if (!ki.insert(mapbox::util::get<KeyInput>(in).keyImage).second)
        return false;
    }
  }

  return true;
}

// TransactionInput helper functions

size_t getRequiredSignaturesCount(const TransactionInput& in) {
  if (in.is<KeyInput>()) {
    return mapbox::util::get<KeyInput>(in).outputIndexes.size();
  }

  return 0;
}

uint64_t getTransactionInputAmount(const TransactionInput& in) {
  if (in.is<KeyInput>()) {
    return mapbox::util::get<KeyInput>(in).amount;
  }

  return 0;
}

TransactionTypes::InputType getTransactionInputType(const TransactionInput& in) {
  if (in.is<KeyInput>()) {
    return TransactionTypes::InputType::Key;
  }

  if (in.is<BaseInput>()) {
    return TransactionTypes::InputType::Generating;
  }

  return TransactionTypes::InputType::Invalid;
}

const TransactionInput& getInputChecked(const CryptoNote::TransactionPrefix& transaction, size_t index) {
  if (transaction.inputs.size() <= index) {
    throw std::runtime_error("Transaction input index out of range");
  }

  return transaction.inputs[index];
}

const TransactionInput& getInputChecked(const CryptoNote::TransactionPrefix& transaction, size_t index, TransactionTypes::InputType type) {
  const auto& input = getInputChecked(transaction, index);
  if (getTransactionInputType(input) != type) {
    throw std::runtime_error("Unexpected transaction input type");
  }

  return input;
}

// TransactionOutput helper functions

TransactionTypes::OutputType getTransactionOutputType(const TransactionOutputTarget& out) {
  if (out.is<KeyOutput>()) {
    return TransactionTypes::OutputType::Key;
  }

  return TransactionTypes::OutputType::Invalid;
}

const TransactionOutput& getOutputChecked(const CryptoNote::TransactionPrefix& transaction, size_t index) {
  if (transaction.outputs.size() <= index) {
    throw std::runtime_error("Transaction output index out of range");
  }

  return transaction.outputs[index];
}

const TransactionOutput& getOutputChecked(const CryptoNote::TransactionPrefix& transaction, size_t index, TransactionTypes::OutputType type) {
  const auto& output = getOutputChecked(transaction, index);
  if (getTransactionOutputType(output.target) != type) {
    throw std::runtime_error("Unexpected transaction output target type");
  }

  return output;
}

bool findOutputsToAccount(const CryptoNote::TransactionPrefix& transaction, const AccountPublicAddress& addr,
                          const SecretKey& viewSecretKey, std::vector<uint32_t>& out, uint64_t& amount) {
  AccountKeys keys;
  keys.address = addr;
  // only view secret key is used, spend key is not needed
  keys.viewSecretKey = viewSecretKey;

  Crypto::PublicKey txPubKey = getTransactionPublicKeyFromExtra(transaction.extra);

  amount = 0;
  size_t keyIndex = 0;
  uint32_t outputIndex = 0;

  Crypto::KeyDerivation derivation;
  generate_key_derivation(txPubKey, keys.viewSecretKey, derivation);

  for (const TransactionOutput& o : transaction.outputs) {
    assert(o.target.is<KeyOutput>());
    if (o.target.is<KeyOutput>()) {
      if (is_out_to_acc(keys, mapbox::util::get<KeyOutput>(o.target), derivation, keyIndex)) {
        out.push_back(outputIndex);
        amount += o.amount;
      }

      ++keyIndex;
    }

    ++outputIndex;
  }

  return true;
}

}
