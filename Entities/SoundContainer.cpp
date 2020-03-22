#include "SoundContainer.h"
#include "RTETools.h"

namespace RTE {

	CONCRETECLASSINFO(SoundContainer, Entity, 0);

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void SoundContainer::Clear() {
		m_Sounds.clear();
		m_SelectedSounds.clear();
		m_PlayingChannels.clear();
		m_Loops = 0;
		m_Priority = AudioMan::PRIORITY_NORMAL;
		m_AffectedByGlobalPitch = true;
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	int SoundContainer::Create(const SoundContainer &reference) {
		Entity::Create(reference);

		for (std::vector<std::pair<ContentFile, FMOD::Sound *>>::const_iterator itr = reference.m_Sounds.begin(); itr != reference.m_Sounds.end(); ++itr) {
			m_Sounds.push_back(*itr);
		}

		m_SelectedSounds = reference.m_SelectedSounds;
		m_PlayingChannels.clear();
		m_Loops = reference.m_Loops;
		m_Priority = reference.m_Priority;
		m_AffectedByGlobalPitch = reference.m_AffectedByGlobalPitch;

		return 0;
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	int SoundContainer::ReadProperty(std::string propName, Reader &reader) {
		if (propName == "AddSample" || propName == "AddSound") {
			ContentFile newFile;
			reader >> newFile;

			FMOD::Sound *pNewSample = newFile.GetAsSample();
			if (!pNewSample) {
				reader.ReportError(std::string("Failed to load the sample from the file"));
			}
			m_Sounds.push_back(std::pair<ContentFile, FMOD::Sound *>(newFile, pNewSample));
		} else if (propName == "LoopSetting") {
			reader >> m_Loops;
		} else if (propName == "Priority") {
			reader >> m_Priority;
			if (m_Priority < 0 || m_Priority > 256) {
				reader.ReportError("SoundContainer priority must be between 256 (lowest priority) and 0 (highest priority).");
			}
		} else if (propName == "AffectedByGlobalPitch") {
			reader >> m_AffectedByGlobalPitch;
		} else {
			// See if the base class(es) can find a match instead
			return Entity::ReadProperty(propName, reader);
		}

		return 0;
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void SoundContainer::AddSound(std::string const soundPath, bool abortGameForInvalidSound) {
		ContentFile newFile(soundPath.c_str());

		FMOD::Sound *newSample = newFile.GetAsSample(abortGameForInvalidSound);
		if (newSample) {
			FMOD_RESULT result = newSample->setMode(m_Loops == 0 ? FMOD_LOOP_OFF : FMOD_LOOP_NORMAL);
			result = result == FMOD_OK ? newSample->setLoopCount(m_Loops) : result;
			m_Sounds.push_back(std::pair<ContentFile, FMOD::Sound *>(newFile, newSample));
		}
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	std::vector<std::size_t> SoundContainer::GetSelectedSoundHashes() {
		std::vector<std::size_t> soundHashes;
		for (std::size_t selectedSoundIndex : m_SelectedSounds) {
			soundHashes.push_back(m_Sounds[selectedSoundIndex].first.GetHash());
		}
		return soundHashes;
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	std::vector<FMOD::Sound *> SoundContainer::GetSelectedSoundObjects() {
 		std::vector<FMOD::Sound *> soundObjects;
		for (std::size_t selectedSoundIndex : m_SelectedSounds) {
			soundObjects.push_back(m_Sounds[selectedSoundIndex].second);
		}
		return soundObjects;
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void SoundContainer::SetLoopSetting(int loops) {
		m_Loops = loops;
		for (std::vector<std::pair<ContentFile, FMOD::Sound * >>::const_iterator itr = m_Sounds.begin(); itr != m_Sounds.end(); ++itr) {
			FMOD_RESULT result = itr->second->setMode(m_Loops == 0 ? FMOD_LOOP_OFF : FMOD_LOOP_NORMAL);
			result = itr->second->setLoopCount(m_Loops);
		}
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	bool SoundContainer::SelectNextSounds() {
		int soundCount = GetSoundCount();
		if (soundCount == 0) {
			return false;
		}

		std::size_t previouslySelectedSound = m_SelectedSounds.size() > 0 ? m_SelectedSounds[0] : 0;

		if (m_SelectedSounds.empty() || soundCount == 1) {
			m_SelectedSounds.clear();
			m_SelectedSounds.push_back(previouslySelectedSound);
		} else {
			m_SelectedSounds.clear();
			if (soundCount == 2) {
				m_SelectedSounds.push_back((previouslySelectedSound + 1) % soundCount);
			} else if (soundCount > 2) {
				size_t soundToSelect = floorf((float)soundCount * PosRand());
				// Mix it up again if we got the same sound twice
				while (soundToSelect == previouslySelectedSound) {
					soundToSelect = floorf((float)soundCount * PosRand());
				}
				m_SelectedSounds.push_back(soundToSelect);
			}
		}
		RTEAssert(m_SelectedSounds.size() > 0 && m_SelectedSounds[0] >= 0 && m_SelectedSounds[0] < soundCount, "Failed to select next sound, either none was selected or the selected sound was invalid.");
		
		return true;
	}
}
