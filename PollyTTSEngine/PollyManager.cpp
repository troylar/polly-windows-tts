/*  Copyright 2017 - 2018 Amazon.com, Inc. or its affiliates.All Rights Reserved.
Licensed under the Amazon Software License(the "License").You may not use
this file except in compliance with the License.A copy of the License is
located at

http://aws.amazon.com/asl/

and in the "LICENSE" file accompanying this file.This file is distributed
on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, express
or implied.See the License for the specific language governing
permissions and limitations under the License. */

#include "stdafx.h"
#include "PollyManager.h"
#include <aws/polly/PollyClient.h>
#include <aws/polly/model/OutputFormat.h>
#include <aws/polly/model/TextType.h>
#include <aws/core/utils/Outcome.h>
#include <aws/core/utils/StringUtils.h>
#include <aws/polly/model/SynthesizeSpeechRequest.h>
#include "TtsEngObj.h"
#include "PollySpeechMarksResponse.h"
#include "rapidjson/document.h"
#include <unordered_map>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/msvc_sink.h"
#include <tinyxml2/tinyxml2.h>
namespace spd = spdlog;

#define NOMINMAX
#ifdef _WIN32
#include <Windows.h>
#endif
#define MAX_SIZE 6000000
using namespace Aws::Polly::Model;

void PollyManager::SetVoice (LPWSTR voiceName)
{
	m_logger->debug("{}: Setting voice to {}", __FUNCTION__, Aws::Utils::StringUtils::FromWString(voiceName));
	m_sVoiceName = voiceName;
	auto voiceId = vm.find(voiceName);
	m_vVoiceId = voiceId->second ;
}

PollyManager::PollyManager(LPWSTR voiceName)
{
#ifdef DEBUG
	m_logger = std::make_shared<spd::logger>("msvc_logger", std::make_shared<spd::sinks::msvc_sink_mt>());
	m_logger->set_level(spd::level::debug);
#endif

	SetVoice(voiceName);
}

PollySpeechResponse PollyManager::GenerateSpeech(CSentItem& item)
{
	PollySpeechResponse response;
	Aws::Polly::PollyClient p;
	SynthesizeSpeechRequest speech_request;
	auto speech_text = Aws::Utils::StringUtils::FromWString(item.pItem);
	m_logger->debug("{}: Asking Polly for '{}'", __FUNCTION__, speech_text.c_str());
	if (Aws::Utils::StringUtils::ToLower(speech_text.c_str()).find("<speak") == 0)
	{
		m_logger->debug("Text type = ssml");
		speech_request.SetTextType(TextType::ssml);
		tinyxml2::XMLDocument sapiDoc;
		sapiDoc.Parse(speech_text.c_str());
		auto elem = sapiDoc.FirstChildElement("speak");
		std::string voiceName;
		while (elem)
		{
			if (!std::string(elem->Value()).compare("voice"))
			{
				voiceName = elem->ToElement()->Attribute("name");
				std::wstring wname;
				wname.assign(voiceName.begin(), voiceName.end());
				auto voiceId = vm.find(wname);
				m_vVoiceId = voiceId->second;
				m_logger->debug(elem->Value());
				auto child = elem->FirstChild();
				while (child)
				{
					m_logger->debug(child->Value());
					sapiDoc.RootElement()->InsertFirstChild(child);
					if (child->NextSibling() && std::string(child->NextSibling()->Value()).compare("voice"))
					{
						child = child->NextSibling();
					}
					else
					{
						sapiDoc.RootElement()->DeleteChild(elem);
						child = NULL;
					}
				}
						break;
			}
			if (elem->FirstChildElement()) {
				elem = elem->FirstChildElement();
			}
			else if (elem->NextSiblingElement()) {
				elem = elem->NextSiblingElement();
			}
			else {
				while (elem && !elem->Parent()->NextSiblingElement()) {
					if (elem->Parent()->ToElement() != sapiDoc.RootElement()) {
						elem = elem->Parent()->NextSiblingElement();
					}
				}
			}
		}
		tinyxml2::XMLPrinter printer;
		sapiDoc.Print(&printer);
		speech_text = printer.CStr();
	}
	else
	{
		m_logger->debug("Text type = text");
		speech_request.SetTextType(TextType::text);
	}
	speech_request.SetOutputFormat(OutputFormat::pcm);
	speech_request.SetVoiceId(m_vVoiceId);

	m_logger->debug("Generating speech: {}", speech_text);
	speech_request.SetText(speech_text);

	speech_request.SetSampleRate("16000");
	auto speech = p.SynthesizeSpeech(speech_request);
	response.IsSuccess = speech.IsSuccess();
	if (!speech.IsSuccess())
	{
		std::stringstream error;
		error << "Error generating speech: " << speech.GetError().GetMessageW();
		response.ErrorMessage = error.str();
		return response;
	}
	auto &r = speech.GetResult();

	auto& stream = r.GetAudioStream();
	stream.read(reinterpret_cast<char*>(&response.AudioData[0]), MAX_SIZE);
	response.Length = stream.gcount();
	return response;
}

std::string PollyManager::ParseXMLOutput(std::string &xmlBuffer)
{
	bool copy = true;
	std::string plainString = "";
	std::stringstream convertStream;

	// remove all xml tags
	for (int i = 0; i < xmlBuffer.length(); i++)
	{
		convertStream << xmlBuffer[i];

		if (convertStream.str().compare("<") == 0) copy = false;
		else if (convertStream.str().compare(">") == 0)
		{
			copy = true;
			convertStream.str(std::string());
			continue;
		}

		if (copy) plainString.append(convertStream.str());

		convertStream.str(std::string());
	}

	return plainString;
}

PollySpeechMarksResponse PollyManager::GenerateSpeechMarks(CSentItem& item, std::streamsize streamSize)
{
	SynthesizeSpeechRequest speechMarksRequest;
	PollySpeechMarksResponse response;
	Aws::Polly::PollyClient p;
	auto text = Aws::Utils::StringUtils::FromWString(item.pItem);
	m_logger->debug("{}: Asking Polly for '{}'", __FUNCTION__, text.c_str());
	speechMarksRequest.SetOutputFormat(OutputFormat::json);
	speechMarksRequest.SetVoiceId(m_vVoiceId);
	speechMarksRequest.SetText(text);
	speechMarksRequest.AddSpeechMarkTypes(SpeechMarkType::word);
	if (Aws::Utils::StringUtils::ToLower(text.c_str()).find("<speak") == 0)
	{
		m_logger->debug("Text type = ssml");
		speechMarksRequest.SetTextType(TextType::ssml);
	}
	else
	{
		m_logger->debug("Text type = text");
		speechMarksRequest.SetTextType(TextType::text);
	}
	speechMarksRequest.SetSampleRate("16000");
	auto speech_marks = p.SynthesizeSpeech(speechMarksRequest);
	if (!speech_marks.IsSuccess())
	{
		std::stringstream error;
		error << "Unable to generate speech marks: " << speech_marks.GetError().GetMessageW();
		response.ErrorMessage = error.str();
		return response;
	}
	auto &m = speech_marks.GetResult();
	auto& m_stream = m.GetAudioStream();
	std::string json_str;
	std::vector<SpeechMark> speechMarks;
	auto firstWord = true;
	long bytesProcessed = 0;
	m_logger->debug("SpeechMarks response:\n\n{}\n\n", json_str);
	while (getline(m_stream, json_str)) {
		SpeechMark sm;
		rapidjson::Document d;
		d.Parse(json_str.c_str());
		assert(d.HasMember("end"));
		assert(d["end"].GetInt());
		sm.StartInMs = d["time"].GetInt();
		sm.StartByte = d["start"].GetInt();
		sm.EndByte = d["end"].GetInt();
		sm.Text = d["value"].GetString();
		SpeechMark displaySpeechMark;
		if (!firstWord)
		{
			auto currentSm = speechMarks[speechMarks.size()-1];
			currentSm.TimeInMs = sm.StartInMs - currentSm.StartInMs;
			currentSm.LengthInBytes = 32 * currentSm.TimeInMs;
			displaySpeechMark = currentSm;
			bytesProcessed += currentSm.LengthInBytes;
			speechMarks[speechMarks.size() - 1] = currentSm;
		}
		m_logger->debug("Word: {}, Start: {}, End: {}, Time: {}\n", sm.Text.c_str(), sm.StartInMs,
			sm.EndByte,
			sm.TimeInMs);
		speechMarks.push_back(sm);
		firstWord = false;
	}
	auto sm = speechMarks[speechMarks.size() - 1];
	sm.LengthInBytes = streamSize - bytesProcessed;
	sm.TimeInMs = sm.LengthInBytes / 32;
	speechMarks[speechMarks.size() - 1] = sm;
	m_logger->debug("Word: {}, Start: {}, End: {}, Time: {}\n", sm.Text.c_str(), sm.StartInMs,
		sm.EndByte,
		sm.TimeInMs);
	m_logger->debug("Total words generated: {}", speechMarks.size());
	speechMarks.push_back(sm);
	response.SpeechMarks = speechMarks;
	return response;
}
