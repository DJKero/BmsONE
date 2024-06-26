#include "Document.h"
#include "SoundChannel.h"
#include "SoundChannelInternal.h"
#include "HistoryUtil.h"
#include "History.h"
#include "MasterCache.h"
#include <cmath>
#include "../bmson/Bmson100.h"
#include "../bms/Bms.h"
#include "../util/ResolutionUtil.h"

using namespace Bmson100;


SoundChannel::SoundChannel(Document *document)
	: QObject(document)
	, document(document)
	, resource(new SoundChannelResourceManager(this))
	, waveSummary()
	, totalLength(0)
{
	connect(resource, SIGNAL(OverallWaveformReady()), this, SLOT(OnOverallWaveformReady()), Qt::QueuedConnection);
	connect(resource, SIGNAL(RmsCacheUpdated()), this, SLOT(OnRmsCacheUpdated()), Qt::QueuedConnection);
	connect(resource, SIGNAL(RmsCachePacketReady(int,QList<RmsCacheEntry>)), this, SLOT(OnRmsCachePacketReady(int,QList<RmsCacheEntry>)), Qt::QueuedConnection);

	connect(document, SIGNAL(TimeMappingChanged()), this, SLOT(OnTimeMappingChanged()));
}

SoundChannel::~SoundChannel()
{
}

void SoundChannel::LoadSound(const QString &filePath)
{
	fileName = document->GetRelativePath(filePath);
	//adjustment = 0.;

	resource->UpdateWaveData(filePath);
	AfterInitialization();
}

void SoundChannel::LoadBmson(const QJsonValue &json)
{
	bmsonFields = json.toObject();
	fileName = bmsonFields[Bmson::SoundChannel::NameKey].toString();
	//adjustment = 0.;
    for (const QJsonValue &jsonNote : bmsonFields[Bmson::SoundChannel::NotesKey].toArray()) {
        SoundNote note(jsonNote);
		notes.insert(note.location, note);
    }

    // temporary length (exact totalLength is calculated in UpdateCache() when whole sound data is available)
	if (notes.empty()){
		totalLength = 0;
	}else{
		totalLength = notes.last().location + notes.last().length;
	}
	resource->UpdateWaveData(document->GetAbsolutePath(fileName));
	AfterInitialization();
}

void SoundChannel::InitializeWithNotes(const QString &name, const QMap<int, SoundNote> &notes)
{
	fileName = name;
	this->notes = notes;

	// temporary length (exact totalLength is calculated in UpdateCache() when whole sound data is available)
	if (notes.empty()){
		totalLength = 0;
	}else{
		totalLength = notes.last().location + notes.last().length;
	}
	resource->UpdateWaveData(document->GetAbsolutePath(fileName));
	AfterInitialization();
}

void SoundChannel::AfterInitialization()
{
	waveSummary = resource->GetWaveSummary();
	emit WaveSummaryUpdated();
	UpdateCache();
	document->ChannelLengthChanged(this, totalLength);
}

QJsonValue SoundChannel::SaveBmson()
{
	bmsonFields[Bmson::SoundChannel::NameKey] = fileName;
	QJsonArray jsonNotes;
	for (SoundNote note : notes){
		jsonNotes.append(note.SaveBmson());
	}
	bmsonFields[Bmson::SoundChannel::NotesKey] = jsonNotes;
	return bmsonFields;
}

void SoundChannel::SetSourceFile(const QString &absolutePath)
{
	QString newFileName = document->GetRelativePath(absolutePath);
	auto setter = [this](QString value){
		AddAllIntoMasterCache(-1);
		fileName = value;
		resource->UpdateWaveData(document->GetAbsolutePath(fileName));
		waveSummary = resource->GetWaveSummary();
		UpdateCache();
		document->ChannelLengthChanged(this, totalLength);
		emit NameChanged();
		emit WaveSummaryUpdated();
		AddAllIntoMasterCache(1);
	};
	auto shower = [this](){
		emit Show();
	};
	auto *action = new EditValueAction<QString>(setter, fileName, newFileName, tr("select sound channel source file"), true, shower);
	document->GetHistory()->Add(action);
}

void SoundChannel::OnOverallWaveformReady()
{
	overallWaveform = resource->GetOverallWaveform();
	emit OverallWaveformUpdated();
}

void SoundChannel::OnRmsCacheUpdated()
{
	// forget missing cache
	for (QMap<int, QList<RmsCacheEntry>>::iterator i=rmsCacheLibrary.begin(); i!=rmsCacheLibrary.end(); ){
		if (i->empty()){
			if (rmsCacheRequestFlag.contains(i.key())){
				rmsCacheRequestFlag.remove(i.key());
			}
			i = rmsCacheLibrary.erase(i);
			continue;
		}
		i++;
	}
	emit RmsUpdated();
}

void SoundChannel::OnRmsCachePacketReady(int position, QList<RmsCacheEntry> packet)
{
	rmsCacheLibrary.insert(position, packet);
	rmsCacheRequestFlag.remove(position);
	emit RmsUpdated();
}

void SoundChannel::OnTimeMappingChanged()
{
	UpdateCache();
	UpdateVisibleRegionsInternal();
}

void SoundChannel::InsertNoteImpl(SoundNote note)
{
	bool updatesMasterCache = note.noteType == 0;
	if (updatesMasterCache){
		MasterCacheAddPreviousNoteInernal(note.location, -1);
	}
	notes.insert(note.location, note);
	UpdateCache();
	UpdateVisibleRegionsInternal();
	if (updatesMasterCache){
		MasterCacheAddPreviousNoteInernal(note.location, 1);
		MasterCacheAddNoteInternal(note.location, 1);
	}
	emit NoteInserted(note);
	document->ChannelLengthChanged(this, totalLength);
}

void SoundChannel::RemoveNoteImpl(SoundNote note)
{
	bool updatesMasterCache = note.noteType == 0;
	if (updatesMasterCache){
		MasterCacheAddPreviousNoteInernal(note.location, -1);
		MasterCacheAddNoteInternal(note.location, -1);
	}
	SoundNote actualNote = notes.take(note.location);
	UpdateCache();
	UpdateVisibleRegionsInternal();
	if (updatesMasterCache){
		MasterCacheAddPreviousNoteInernal(note.location, 1);
	}
	emit NoteRemoved(actualNote);
	document->ChannelLengthChanged(this, totalLength);
}

void SoundChannel::UpdateNoteImpl(SoundNote note)
{
	bool updatesMasterCache = note.noteType == 0 || notes[note.location].noteType == 0;
	if (updatesMasterCache){
		MasterCacheAddPreviousNoteInernal(note.location, -1);
		MasterCacheAddNoteInternal(note.location, -1);
	}
	notes[note.location] = note;
	UpdateCache();
	UpdateVisibleRegionsInternal();
	if (updatesMasterCache){
		MasterCacheAddPreviousNoteInernal(note.location, 1);
		MasterCacheAddNoteInternal(note.location, 1);
	}
	emit NoteChanged(note.location, note);
	document->ChannelLengthChanged(this, totalLength);
}

EditAction *SoundChannel::InsertNoteInternal(SoundNote note, bool notifyByDocument, UpdateNotePolicy policy, QList<int> acceptableLanes)
{
retry:
	// check lane conflict
	if (note.lane == 0){
		if (notes.contains(note.location) && notes[note.location].lane == 0){
			return nullptr;
		}
	}else{
		auto conflictingNotes = document->FindConflictingNotes(note);
		auto pred = [=](QPair<SoundChannel*, int> pair){ return pair.first != this || pair.second != note.location; };
		if (std::any_of(conflictingNotes.begin(), conflictingNotes.end(), pred)){
			// indeed conflicts
			switch (policy){
			case UpdateNotePolicy::Conservative:
				return nullptr;
			case UpdateNotePolicy::BestEffort:
				if (acceptableLanes.empty())
					return nullptr;
				note.lane = acceptableLanes.front();
				acceptableLanes.pop_front();
				goto retry;
			case UpdateNotePolicy::ForceMove:
				break;
			}
		}else if (!conflictingNotes.empty()){
			if (this->notes[note.location] == note){
				// update not needed
				return nullptr;
			}
		}
	}
	auto shower = [=](){
		emit ShowNoteLocation(note.location);
	};
	if (notes.contains(note.location)){
		// move
		auto setter = [notifyByDocument,this](SoundNote note){
			UpdateNoteImpl(note);
			if (notifyByDocument){
				emit document->AnyNotesChanged();
			}
		};
		return new EditValueAction<SoundNote>(setter, notes[note.location], note, tr("modify sound note"), true, shower);
	}else{
		// new
		auto adder = [notifyByDocument,this](SoundNote note){
			InsertNoteImpl(note);
			if (notifyByDocument){
				emit document->AnyNotesChanged();
			}
		};
		auto remover = [notifyByDocument,this](SoundNote note){
			RemoveNoteImpl(note);
			if (notifyByDocument){
				emit document->AnyNotesChanged();
			}
		};
		return new AddValueAction<SoundNote>(adder, remover, note, tr("add sound note"), true, shower);
	}
}

EditAction *SoundChannel::RemoveNoteInternal(SoundNote note, bool notifyByDocument)
{
	if (!notes.contains(note.location))
		return nullptr;
	auto shower = [=](){
		emit ShowNoteLocation(note.location);
	};
	auto adder = [notifyByDocument,this](SoundNote note){
		InsertNoteImpl(note);
		if (notifyByDocument){
			emit document->AnyNotesChanged();
		}
	};
	auto remover = [notifyByDocument,this](SoundNote note){
		RemoveNoteImpl(note);
		if (notifyByDocument){
			emit document->AnyNotesChanged();
		}
	};
	return new RemoveValueAction<SoundNote>(adder, remover, note, tr("remove sound note"), true, shower);
}


bool SoundChannel::InsertNote(SoundNote note)
{
	EditAction *action = InsertNoteInternal(note);
	if (action == nullptr){
		return false;
	}
	document->GetHistory()->Add(action);
	return true;
}

bool SoundChannel::RemoveNote(SoundNote note)
{
	EditAction *action = RemoveNoteInternal(note);
	if (action == nullptr){
		return false;
	}
	document->GetHistory()->Add(action);
	return true;
}

void SoundChannel::UpdateVisibleRegions(const QList<QPair<int, int> > &visibleRegionsTime)
{
	visibleRegions = visibleRegionsTime;
	UpdateVisibleRegionsInternal();
}

void SoundChannel::UpdateVisibleRegionsInternal()
{
	if (!waveSummary.IsValid() || visibleRegions.empty()){
		rmsCacheLibrary.clear();
		return;
	}
	const double samplesPerSec = waveSummary.Format.sampleRate();
	const double ticksPerBeat = document->GetInfo()->GetResolution();
	QMultiMap<int, int> regions; // key:start value:end

	QMutexLocker lock(&cacheMutex);
	for (QPair<int, int> reg : visibleRegions){
		const int endAt = reg.second;
		int ticks = reg.first;
		QMap<int, CacheEntry>::const_iterator icache = cache.lowerBound(reg.first);
		if (icache == cache.end())
			break;
		if (icache->prevSamplePosition >= 0){
			// first interval
			double pos = icache->prevSamplePosition - (icache.key() - ticks)*(60.0 * samplesPerSec / (icache->prevTempo * ticksPerBeat));
			if (icache.key() >= endAt){
				// reg is in single interval
				double posEnd = icache->prevSamplePosition - (icache.key() - endAt)*(60.0 * samplesPerSec / (icache->prevTempo * ticksPerBeat));
				regions.insert(pos, posEnd);
				continue;
			}
			regions.insert(pos, icache->prevSamplePosition);
		}
		ticks = icache.key();
		int pos = icache->currentSamplePosition;
		icache++;
		while (icache != cache.end() && icache.key() < endAt){
			// middle intervals
			int posEnd = icache->prevSamplePosition;
			if (pos >= 0 && posEnd >= 0){
				regions.insert(pos, posEnd);
			}
			ticks = icache.key();
			pos = icache->currentSamplePosition;
			icache++;
		}
		if (pos >= 0 && icache->prevSamplePosition >= 0){
			// last interval
			double posEnd = icache->prevSamplePosition - (icache.key() - endAt)*(60.0 * samplesPerSec / (icache->prevTempo * ticksPerBeat));
			regions.insert(pos, posEnd);
		}
	}

	// merge overlapped regions
	if (regions.empty()){
		rmsCacheLibrary.clear();
		return;
	}
	QMap<int, int> merged = SoundChannelUtil::MergeRegions(regions);

	// unload invisible packets and (request to) load visible packets
	int posLast = merged.lastKey() + merged.last();
	if (!rmsCacheLibrary.empty()){
		posLast = std::max(posLast, rmsCacheLibrary.lastKey() + SoundChannelResourceManager::RmsCachePacketSampleCount);
	}
	int packetIxEnd = (posLast - 1)/SoundChannelResourceManager::RmsCachePacketSampleCount + 1;
	QSet<int> visiblePacketKeys;
	for (QMap<int, int>::const_iterator iVisible=merged.begin(); iVisible!=merged.end(); iVisible++){
		int ixBegin = iVisible.key() / SoundChannelResourceManager::RmsCachePacketSampleCount;
		int ixEnd = (iVisible.value()-1) / SoundChannelResourceManager::RmsCachePacketSampleCount + 1;
		for (int ix=ixBegin; ix<ixEnd; ix++){
			visiblePacketKeys.insert(ix * SoundChannelResourceManager::RmsCachePacketSampleCount);
		}
	}
	for (int ix=0; ix<packetIxEnd; ix++){
		int pos = ix*SoundChannelResourceManager::RmsCachePacketSampleCount;
		QMap<int, QList<RmsCacheEntry>>::iterator i = rmsCacheLibrary.find(pos);
		if (visiblePacketKeys.contains(pos)){
			if (i == rmsCacheLibrary.end() && !rmsCacheRequestFlag.contains(pos)){
				rmsCacheRequestFlag.insert(pos, true);
				resource->RequireRmsCachePacket(pos);
			}
		}else if (i != rmsCacheLibrary.end()){
			rmsCacheLibrary.erase(i);
		}
	}
}


void SoundChannel::DrawRmsGraph(double location, double resolution, std::function<bool(Rms)> drawer) const
{
	if (!waveSummary.IsValid()){
		return;
	}
	const double samplesPerSec = waveSummary.Format.sampleRate();
	const double ticksPerBeat = document->GetInfo()->GetResolution();
	const double deltaTicks = 1 / resolution;
	double ticks = location;
	QMutexLocker lock(&cacheMutex);
	QMap<int, CacheEntry>::const_iterator icache = cache.lowerBound(location);
	double pos = icache->prevSamplePosition - (icache.key() - ticks)*(60.0 * samplesPerSec / (icache->prevTempo * ticksPerBeat));
	while (icache != cache.end()){
		double nextPos = icache->prevSamplePosition - (icache.key() - ticks)*(60.0 * samplesPerSec / (icache->prevTempo * ticksPerBeat));
		int iPos = pos + 0.5;
		int iNextPos = nextPos + 0.5;
		ticks += deltaTicks;
		if (iPos >= waveSummary.FrameCount|| iNextPos <= 0){
			if (!drawer(Rms())){
				return;
			}
		}else{
			if (iPos < 0){
				iPos = 0;
			}
			if (iNextPos > waveSummary.FrameCount){
				iNextPos = waveSummary.FrameCount;
			}
			if (iPos >= iNextPos){
				if (!drawer(Rms())){
					return;
				}
			}else{
				int ixBegin = pos / SoundChannelResourceManager::RmsCachePacketSampleCount;
				int ixPacket = ixBegin * SoundChannelResourceManager::RmsCachePacketSampleCount;
				auto itRms = rmsCacheLibrary.find(ixPacket);
				if (itRms != rmsCacheLibrary.end() && itRms->size() > 0){
					const QList<RmsCacheEntry> &entries = *itRms;
					int bxBegin = std::max(0, std::min(entries.size()-1, (iPos - ixPacket) / SoundChannelResourceManager::RmsCacheBlockSize));
					int bxEnd = std::max(bxBegin+1, std::min(entries.size(), (iNextPos - ixPacket) / SoundChannelResourceManager::RmsCacheBlockSize));
					// assume (bxEnd-bxBegin) < 2^16
					unsigned int rmsL=0;
					unsigned int rmsR=0;
					for (int b=bxBegin; b!=bxEnd; b++){
						rmsL += entries[b].L*entries[b].L;
						rmsR += entries[b].R*entries[b].R;
					}
                    if (!drawer(Rms(std::sqrt(float(rmsL) / (bxEnd-bxBegin)) / 127.f, std::sqrt(float(rmsR) / (bxEnd-bxBegin)) / 127.f))){
						return;
					}
				}else{
					// no cache
					if (!drawer(Rms())){
						return;
					}
				}
			}
		}
		pos = nextPos;
		while (ticks > icache.key()){
			icache++;
		}
	}
	while (drawer(Rms()));
}

void SoundChannel::DrawActivityGraph(double tBegin, double tEnd, std::function<void(bool, int, int)> drawer) const
{
	if (!waveSummary.IsValid()){
		return;
	}
	if (tBegin >= tEnd)
		return;
	QMutexLocker lock(&cacheMutex);

	auto icache = cache.upperBound(tBegin);
	auto icacheEnd = cache.upperBound(tEnd);
	if (icache != cache.begin())
		icache--;
	while (icache != icacheEnd){
		// 音声が鳴り始めるまでスキップ
		while (icache != icacheEnd && icache.value().currentSamplePosition < 0)
			icache++;
		if (icache == icacheEnd)
			break;
		int time = icache.key();
		// 鳴り終わり（またはtEnd超え）の時刻を取得（サンプル位置が0に戻るのはスルー）
		while (icache != icacheEnd && icache.value().currentSamplePosition >= 0)
			icache++;
		int timeEnd = icache == icacheEnd ? tEnd : icache.key();
		// 鳴り始めのノーツレーン種別を取得
		auto inote = notes.upperBound(time);
		auto inoteEnd = notes.upperBound(timeEnd);
		if (inote != notes.begin())
			inote--;
		bool laneType = inote->lane > 0;
		// 鳴り終わりまでにわたり、レーン種別の変化を追いかけながら描画
		while (++inote != inoteEnd){
			bool newLaneType = inote->lane > 0;
			if (newLaneType != laneType){
				drawer(laneType, time, inote.key());
				time = inote.key();
				laneType = newLaneType;
			}
		}
		// 残りの部分（timeからtimeEndまで）を埋める
		drawer(laneType, time, timeEnd);
	}
}

bool SoundChannel::IsActiveInRegion(int tBegin, int tEnd) const
{
	if (!waveSummary.IsValid()){
		return false;
	}
	if (tBegin >= tEnd)
		return false;
	// 音声が鳴っているかどうかを検出
	QMutexLocker lock(&cacheMutex);
	auto icache = cache.upperBound(tBegin);
	auto icacheEnd = cache.upperBound(tEnd);
	if (icache != cache.begin())
		icache--;
	while (icache != icacheEnd){
		if (icache.value().currentSamplePosition >= 0)
			return true;
		icache++;
	}
	// ノーツ（無音）があるかどうかも検出
	if (notes.upperBound(tBegin) != notes.upperBound(tEnd))
		return true;
	return false;
}

QSet<int> SoundChannel::GetAllLocations() const
{
	QSet<int> locs;
	for (auto note : notes){
		locs.insert(note.location);
		if (note.length != 0){
			locs.insert(note.location + note.length);
		}
	}
	return locs;
}

void SoundChannel::ConvertResolution(int newResolution, int oldResolution)
{
	QMap<int, SoundNote> notesOld = notes;
	notes.clear();
	for (auto note : notesOld){
		note.location = ResolutionUtil::ConvertTicks(note.location, newResolution, oldResolution);
		note.length = ResolutionUtil::ConvertTicks(note.length, newResolution, oldResolution);
		notes.insert(note.location, note);
	}
	UpdateCache();
}

void SoundChannel::AddAllIntoMasterCache(int sign)
{
	QMutexLocker lock(&cacheMutex);
	QList<MasterCacheMultiWorker::Patch> patches;
	for (auto note : notes){
		if (note.noteType != 0)
			continue;
		QMap<int, CacheEntry>::const_iterator icache = cache.find(note.location);
		if (icache == cache.end()){
			//qDebug() << "Cache entry not found!" << note.location;
			continue;
		}
		while (++icache != cache.end()){
			if (icache->currentSamplePosition != icache->prevSamplePosition){
				MasterCacheMultiWorker::Patch patch;
				patch.sign = sign;
				patch.time = document->GetAbsoluteTime(note.location) * MasterCache::SampleRate;
				patch.frames = double(icache->prevSamplePosition) * MasterCache::SampleRate / waveSummary.Format.sampleRate();
				patches.append(patch);
				break;
			}
		}
	}
	if (patches.size() > 0){
		document->GetMaster()->MultiAddSound(patches, this);
	}
}

void SoundChannel::MasterCacheAddPreviousNoteInernal(int location, int v)
{
	// check if there exists the previous note cut by the note[location].
	{
		QMap<int, CacheEntry>::const_iterator icache = cache.lowerBound(location);
		if (icache == cache.end() || icache->prevSamplePosition < 0)
			return; // no previous note cut by the note[location].
	}
	auto inote = notes.lowerBound(location);
	//if (inote == notes.begin())
	//	return; // cannot happen
	do{
		inote--;
	}while(inote != notes.begin() && inote->noteType != 0);
	//if (inote->noteType != 0)
	//	return; // cannot happen
	MasterCacheAddNoteInternal(inote.key(), v);
}

void SoundChannel::MasterCacheAddNoteInternal(int location, int v)
{
	auto note = notes[location];
	if (note.noteType != 0)
		return;
	QMap<int, CacheEntry>::const_iterator icache = cache.find(location);
	if (icache == cache.end()){
		//qDebug() << "Cache entry not found!" << note.location;
		return;
	}
	while (++icache != cache.end()){
		if (icache->currentSamplePosition != icache->prevSamplePosition){
			document->GetMaster()->AddSound(
						document->GetAbsoluteTime(location) * MasterCache::SampleRate,
						v, this,
						double(icache->prevSamplePosition) * MasterCache::SampleRate / waveSummary.Format.sampleRate());
			return;
		}
	}
}

void SoundChannel::UpdateCache()
{
	QMutexLocker lock(&cacheMutex);
	cache.clear();
	if (!waveSummary.IsValid()){
		return;
	}
	QMap<int, SoundNote>::const_iterator iNote = notes.begin();
	QMap<int, BpmEvent>::const_iterator iTempo = document->GetBpmEvents().begin();
	for (; iNote != notes.end() && iNote->noteType != 0; iNote++);
	int loc = 0;
	int soundEndsAt = -1;
	CacheEntry entry;
	entry.currentSamplePosition = -1;
	entry.currentTempo = document->GetInfo()->GetInitBpm();
	const double samplesPerSec = waveSummary.Format.sampleRate();
	const double ticksPerBeat = document->GetInfo()->GetResolution();
	double currentSamplesPerTick = samplesPerSec * 60.0 / (entry.currentTempo * ticksPerBeat);
	while (true){
		if (iNote != notes.end() && (iTempo == document->GetBpmEvents().end() || iNote->location < iTempo->location)){
			if (entry.currentSamplePosition < 0){
				// START
				entry.prevSamplePosition = -1;
				entry.currentSamplePosition = 0;
				entry.prevTempo = entry.currentTempo;
				loc = iNote->location;
				cache.insert(loc, entry);
				//if (this->GetName() == "d_cym")qDebug() << "insert: START " << loc;
				soundEndsAt = loc + int(waveSummary.FrameCount / currentSamplesPerTick);
				for (iNote++; iNote != notes.end() && iNote->noteType != 0; iNote++);
			}else if (soundEndsAt < iNote->location){
				// END before RESTART
				entry.prevSamplePosition = waveSummary.FrameCount;
				entry.prevTempo = entry.currentTempo;
				entry.currentSamplePosition = -1;
				loc = soundEndsAt;
				cache.insert(loc, entry);
				//if (this->GetName() == "d_cym")qDebug() << "insert: END before RESTART " << loc;
			}else{
				// RESTART before END
				qint64 sp = entry.currentSamplePosition + qint64((iNote.key() - loc) * currentSamplesPerTick);
				entry.prevSamplePosition = std::min(sp, waveSummary.FrameCount);
				entry.prevTempo = entry.currentTempo;
				entry.currentSamplePosition = 0;
				loc = iNote->location;
				cache.insert(loc, entry);
				//if (this->GetName() == "d_cym")qDebug() << "insert: RESTART before END " << loc;
				soundEndsAt = loc + int(waveSummary.FrameCount / currentSamplesPerTick);
				for (iNote++; iNote != notes.end() && iNote->noteType != 0; iNote++);
			}
		}else if (iNote != notes.end() && iTempo != document->GetBpmEvents().end() && iNote->location == iTempo->location){
			if (entry.currentSamplePosition < 0){
				// START & BPM
				entry.prevSamplePosition = -1;
				entry.currentSamplePosition = 0;
				entry.prevTempo = entry.currentTempo;
				entry.currentTempo = iTempo->value;
				currentSamplesPerTick = samplesPerSec * 60.0 / (iTempo->value * ticksPerBeat);
				loc = iNote->location;
				cache.insert(loc, entry);
				//if (this->GetName() == "d_cym")qDebug() << "insert: START & BPM " << loc;
				soundEndsAt = loc + int(waveSummary.FrameCount / currentSamplesPerTick);
				for (iNote++; iNote != notes.end() && iNote->noteType != 0; iNote++);
				iTempo++;
			}else if (soundEndsAt < iNote->location){
				// END before RESTART & BPM
				entry.prevSamplePosition = waveSummary.FrameCount;
				entry.prevTempo = entry.currentTempo;
				entry.currentSamplePosition = -1;
				loc = soundEndsAt;
				cache.insert(loc, entry);
				//if (this->GetName() == "d_cym")qDebug() << "insert: END before RESTART & BPM " << loc;
			}else{
				// RESTART & BPM before END
				qint64 sp = entry.currentSamplePosition + qint64((iNote.key() - loc) * currentSamplesPerTick);
				entry.prevSamplePosition = std::min(sp, waveSummary.FrameCount);
				entry.prevTempo = entry.currentTempo;
				entry.currentSamplePosition = 0;
				entry.currentTempo = iTempo->value;
				currentSamplesPerTick = samplesPerSec * 60.0 / (iTempo->value * ticksPerBeat);
				loc = iNote->location;
				cache.insert(loc, entry);
				//if (this->GetName() == "d_cym")qDebug() << "insert: RESTART & BPM before END " << loc;
				soundEndsAt = loc + int(waveSummary.FrameCount / currentSamplesPerTick);
				for (iNote++; iNote != notes.end() && iNote->noteType != 0; iNote++);
				iTempo++;
			}
		}else if (iTempo != document->GetBpmEvents().end()){
			if (entry.currentSamplePosition < 0){
				// BPM (no sound)
				entry.prevSamplePosition = -1;
				entry.prevTempo = entry.currentTempo;
				entry.currentTempo = iTempo->value;
				currentSamplesPerTick = samplesPerSec * 60.0 / (iTempo->value * ticksPerBeat);
				loc = iTempo->location;
				cache.insert(loc, entry);
				//if (this->GetName() == "d_cym")qDebug() << "insert: BPM (no sound) " << loc;
				iTempo++;
			}else if (soundEndsAt < iTempo.key()){
				// END before BPM
				entry.prevSamplePosition = waveSummary.FrameCount;
				entry.prevTempo = entry.currentTempo;
				entry.currentSamplePosition = -1;
				loc = soundEndsAt;
				cache.insert(loc, entry);
				//if (this->GetName() == "d_cym")qDebug() << "insert: END before BPM " << loc;
			}else{
				// BPM during playing
				qint64 sp = entry.currentSamplePosition + qint64((iTempo.key() - loc) * currentSamplesPerTick);
				entry.prevSamplePosition = std::min(sp, waveSummary.FrameCount);
				entry.prevTempo = entry.currentTempo;
				entry.currentSamplePosition = entry.prevSamplePosition;
				entry.currentTempo = iTempo->value;
				currentSamplesPerTick = samplesPerSec * 60.0 / (iTempo->value * ticksPerBeat);
				loc = iTempo->location;
				cache.insert(loc, entry);
				//if (this->GetName() == "d_cym")qDebug() << "insert: BPM during Playing " << loc << "[" << entry.prevTempo << entry.currentTempo << "]" <<
				//										   "(" << entry.prevSamplePosition << entry.currentSamplePosition << ")";
				soundEndsAt = loc + int((waveSummary.FrameCount - entry.currentSamplePosition) / currentSamplesPerTick);
				iTempo++;
			}
		}else{
			// no iNote, iTempo
			if (entry.currentSamplePosition < 0){
				// no sound
				break;
			}else if (loc == soundEndsAt){
				// sound end & previous action (BPM?) simultaneous
				QMap<int, CacheEntry>::iterator iCache = cache.find(loc);
				if (iCache == cache.end()){
					entry.prevSamplePosition = waveSummary.FrameCount;
					entry.prevTempo = entry.currentTempo;
					entry.currentSamplePosition = -1;
					loc = soundEndsAt;
					cache.insert(loc, entry);
					//if (this->GetName() == "d_cym")qDebug() << "insert: END when bpm change ?" << loc;
					break;
				}else{
					iCache->currentSamplePosition = -1;
					//if (this->GetName() == "d_cym")qDebug() << "insert: END when bpm change " << loc;
					break;
				}
			}else{
				// all we have to do is finish sound
				entry.prevSamplePosition = waveSummary.FrameCount;
				entry.prevTempo = entry.currentTempo;
				entry.currentSamplePosition = -1;
				loc = soundEndsAt;
				cache.insert(loc, entry);
				//if (this->GetName() == "d_cym")qDebug() << "insert: END " << loc;
				break;
			}
		}
	}

	totalLength = 0;
	if (!cache.isEmpty() && totalLength < cache.lastKey()){
		totalLength = cache.lastKey();
	}
	if (!notes.isEmpty() && totalLength < notes.lastKey()){
		totalLength = notes.lastKey();
	}

	entry.prevSamplePosition = entry.currentSamplePosition = -1;
	entry.prevTempo = entry.currentTempo;
	cache.insert(INT_MAX, entry);
	/*if (this->GetName() == "d_cym"){
		qDebug() << "-------------";
		for (QMap<int, CacheEntry>::iterator i=cache.begin(); i!=cache.end(); i++){
			qDebug() << i.key() << i->prevSamplePosition << i->currentSamplePosition << i->prevTempo << i->currentTempo;
		}
		qDebug() << "-------------";
	}*/
}


SoundNote::SoundNote(const QJsonValue &json)
	: BmsonObject(json)
{
	lane = bmsonFields[Bmson::Note::LaneKey].toInt();
	location = bmsonFields[Bmson::Note::LocationKey].toInt();
	length = bmsonFields[Bmson::Note::LengthKey].toInt();
	noteType = bmsonFields[Bmson::Note::ContinueKey].toBool() ? 1 : 0;
}

QJsonValue SoundNote::SaveBmson()
{
	bmsonFields[Bmson::Note::LaneKey] = lane;
	bmsonFields[Bmson::Note::LocationKey] = location;
	bmsonFields[Bmson::Note::LengthKey] = length;
	bmsonFields[Bmson::Note::ContinueKey] = noteType > 0;
	return bmsonFields;
}

QMap<QString, QJsonValue> SoundNote::GetExtraFields() const
{
	QMap<QString, QJsonValue> fields;
	for (QJsonObject::const_iterator i=bmsonFields.begin(); i!=bmsonFields.end(); i++){
		if (i.key() != Bmson::Note::LocationKey
			&& i.key() != Bmson::Note::LaneKey
			&& i.key() != Bmson::Note::LengthKey
			&& i.key() != Bmson::Note::ContinueKey)
		{
			fields.insert(i.key(), i.value());
		}
	}
	return fields;
}

void SoundNote::SetExtraFields(const QMap<QString, QJsonValue> &fields)
{
	bmsonFields = QJsonObject();
	for (auto i=fields.begin(); i!=fields.end(); i++){
		if (i.key() != Bmson::Note::LocationKey
			&& i.key() != Bmson::Note::LaneKey
			&& i.key() != Bmson::Note::LengthKey
			&& i.key() != Bmson::Note::ContinueKey)
		{
			bmsonFields[i.key()] = i.value();
		}
	}
}

QJsonObject SoundNote::AsJson() const
{
	QJsonObject obj = bmsonFields;
	obj[Bmson::Note::LaneKey] = lane;
	obj[Bmson::Note::LocationKey] = location;
	obj[Bmson::Note::LengthKey] = length;
	obj[Bmson::Note::ContinueKey] = noteType > 0;
	return obj;
}


bool NoteConflict::IsMainNote(SoundChannel *channel, SoundNote note) const
{
	for (auto pair : involvedNotes){
		if (pair.first == channel && pair.second == note)
			return true;
		if (pair.second.location == location)
			return false;
	}
	return false;
}
