#include "Document.h"
#include "History.h"
#include "HistoryUtil.h"
#include "../bmson/Bmson100.h"
#include "../bms/Bms.h"

using namespace Bmson100;

QSet<QString> DocumentInfo::SupportedKeys;

DocumentInfo::DocumentInfo(Document *document)
	: QObject(document)
	, document(document)
	, resolution(DefaultResolution)
{
	if (SupportedKeys.isEmpty()){
        SupportedKeys.insert(Bmson::BmsonInfo::TitleKey);
        SupportedKeys.insert(Bmson::BmsonInfo::SubtitleKey);
        SupportedKeys.insert(Bmson::BmsonInfo::GenreKey);
        SupportedKeys.insert(Bmson::BmsonInfo::ArtistKey);
        SupportedKeys.insert(Bmson::BmsonInfo::SubartistsKey);
        SupportedKeys.insert(Bmson::BmsonInfo::ChartNameKey);
        SupportedKeys.insert(Bmson::BmsonInfo::ModeHintKey);
        SupportedKeys.insert(Bmson::BmsonInfo::ResolutionKey);
        SupportedKeys.insert(Bmson::BmsonInfo::JudgeRankKey);
        SupportedKeys.insert(Bmson::BmsonInfo::TotalKey);
        SupportedKeys.insert(Bmson::BmsonInfo::InitBpmKey);
        SupportedKeys.insert(Bmson::BmsonInfo::LevelKey);
        SupportedKeys.insert(Bmson::BmsonInfo::BackImageKey);
        SupportedKeys.insert(Bmson::BmsonInfo::EyecatchImageKey);
        SupportedKeys.insert(Bmson::BmsonInfo::TitleImageKey);
        SupportedKeys.insert(Bmson::BmsonInfo::BannerKey);
        SupportedKeys.insert(Bmson::BmsonInfo::PreviewMusicKey);
	}
}

DocumentInfo::~DocumentInfo()
{
}

void DocumentInfo::Initialize()
{
	title = QString();
	subtitle = QString();
	genre = QString();
	artist = QString();
	subartists.clear();
	chartName = QString();
    modeHint = "ez2-5k";
	resolution = DefaultResolution;
	judgeRank = 100.;
	total = 100.;
	initBpm = 120.;
	level = 1;
	backImage = QString();
	eyecatchImage = QString();
	titleImage = QString();
	banner = QString();
	previewMusic = QString();
}

void DocumentInfo::LoadBmson(QJsonValue json)
{
	bmsonFields = json.toObject();
    title = bmsonFields[Bmson::BmsonInfo::TitleKey].toString();
    subtitle = bmsonFields[Bmson::BmsonInfo::SubtitleKey].toString();
    genre = bmsonFields[Bmson::BmsonInfo::GenreKey].toString();
    artist = bmsonFields[Bmson::BmsonInfo::ArtistKey].toString();
	subartists.clear();
    auto tmp_subartists = bmsonFields[Bmson::BmsonInfo::SubartistsKey].toArray();
	for (auto entry : tmp_subartists){
		subartists.append(entry.toString());
	}
    chartName = bmsonFields[Bmson::BmsonInfo::ChartNameKey].toString();
    modeHint = bmsonFields[Bmson::BmsonInfo::ModeHintKey].toString();
    resolution = bmsonFields[Bmson::BmsonInfo::ResolutionKey].toInt();
	if (resolution <= 0 || resolution > 24000){
		resolution = DefaultResolution;
	}
    judgeRank = bmsonFields[Bmson::BmsonInfo::JudgeRankKey].toDouble();
    total = bmsonFields[Bmson::BmsonInfo::TotalKey].toDouble();
    initBpm = bmsonFields[Bmson::BmsonInfo::InitBpmKey].toDouble();
    level = bmsonFields[Bmson::BmsonInfo::LevelKey].toInt();
    backImage = bmsonFields[Bmson::BmsonInfo::BackImageKey].toString();
    eyecatchImage = bmsonFields[Bmson::BmsonInfo::EyecatchImageKey].toString();
    titleImage = bmsonFields[Bmson::BmsonInfo::TitleImageKey].toString();
    banner = bmsonFields[Bmson::BmsonInfo::BannerKey].toString();
    previewMusic = bmsonFields[Bmson::BmsonInfo::PreviewMusicKey].toString();
}

void DocumentInfo::LoadBms(const Bms::Bms &bms)
{
	Initialize();
	title = bms.title;
	subtitle = bms.subtitle;
	genre = bms.genre;
	artist = bms.artist;
	if (!bms.subartist.isEmpty()){
		subartists.append(bms.subartist);
	}
	switch (bms.mode){
	case Bms::MODE_5K:
		modeHint = "beat-5k";
		break;
	case Bms::MODE_7K:
		modeHint = "beat-7k";
		break;
	case Bms::MODE_10K:
		modeHint = "beat-10k";
		break;
	case Bms::MODE_14K:
		modeHint = "beat-14k";
		break;
	case Bms::MODE_PMS_9K:
		modeHint = "popn-9k";
		break;
	case Bms::MODE_PMS_5K:
		modeHint = "popn-5k";
		break;
	}
	switch (bms.judgeRank){
	case 0:
		judgeRank = 40.0;
		break;
	case 1:
		judgeRank = 60.0;
		break;
	case 2:
		judgeRank = 80.0;
		break;
	case 3:
		judgeRank = 100.0;
		break;
	case 4:
		judgeRank = 120.0;
		break;
	}
	// total =
	initBpm = bms.bpm;
	level = bms.level;
	// backImage =
	eyecatchImage = bms.stageFile;
	titleImage = bms.backBmp;
	banner = bms.banner;
	// previewMusic =

	Bms::BmsReaderConfig bmsConfig;
	bmsConfig.Load();

	// 最後のオブジェの位置がint型に十分収まるように解像度の上限を求める
	qreal totalLength = Bms::BmsUtil::GetTotalLength(bms);
	qDebug() << "total length:" << totalLength;
	int maxResolution = INT_MAX / totalLength / 2;
	qDebug() << "max resolution by total length:" << maxResolution;

	// 曲の長さによる解像度の制約が弱い場合も、常識的な範囲内に収まるように制限を加える (設定で変更できるべき)
	maxResolution = std::min(bmsConfig.maximumResolution, maxResolution);

	bool shrink;
	resolution = Bms::BmsUtil::GetResolution(bms, maxResolution, &shrink);
	qDebug() << "required resolution:" << resolution << " shrink:" << shrink;
	// 必要解像度が低い場合、最低解像度と同等以上になるように引き上げる (素因数の優先順位は適当) // 240 = 16 * 3 * 5
	static QList<int> referenceResolutionFactors = {
		2,	// 2
		2,	// 4
		2,	// 8
		3,	// 24
		2,	// 48
		5,	// 240
		2,	// 480
		2,	// 960
		3,	// 2880
		7,	// 20160
		2,	// 40320
	};
	for (int i=0, r=1; i<referenceResolutionFactors.length() && resolution < bmsConfig.minimumResolution; i++){
		r *= referenceResolutionFactors[i];
		resolution = Bms::Math::LCM(resolution, r);
	}
	qDebug() << "final resolution:" << resolution;
}

QJsonValue DocumentInfo::SaveBmson()
{
    bmsonFields[Bmson::BmsonInfo::TitleKey] = title;
    bmsonFields[Bmson::BmsonInfo::SubtitleKey] = subtitle;
    bmsonFields[Bmson::BmsonInfo::GenreKey] = genre;
    bmsonFields[Bmson::BmsonInfo::ArtistKey] = artist;
	auto tmp_subartists = QJsonArray();
	for (auto entry : subartists){
		tmp_subartists.append(entry);
	}
    bmsonFields[Bmson::BmsonInfo::SubartistsKey] = tmp_subartists;
    bmsonFields[Bmson::BmsonInfo::ChartNameKey] = chartName;
    bmsonFields[Bmson::BmsonInfo::ModeHintKey] = modeHint;
    bmsonFields[Bmson::BmsonInfo::ResolutionKey] = resolution;
    bmsonFields[Bmson::BmsonInfo::JudgeRankKey] = judgeRank;
    bmsonFields[Bmson::BmsonInfo::TotalKey] = total;
    bmsonFields[Bmson::BmsonInfo::InitBpmKey] = initBpm;
    bmsonFields[Bmson::BmsonInfo::LevelKey] = level;
    bmsonFields[Bmson::BmsonInfo::BackImageKey] = backImage;
    bmsonFields[Bmson::BmsonInfo::EyecatchImageKey] = eyecatchImage;
    bmsonFields[Bmson::BmsonInfo::TitleImageKey] = titleImage;
    bmsonFields[Bmson::BmsonInfo::BannerKey] = banner;
    bmsonFields[Bmson::BmsonInfo::PreviewMusicKey] = previewMusic;
	return bmsonFields;
}

void DocumentInfo::SetTitleInternal(QString value){
	title = value;
	emit TitleChanged(title);
}

void DocumentInfo::SetTitle(QString value){
	if (value == title)
		return;
	document->GetHistory()->Add(new EditValueAction<QString>([this](QString v){ SetTitleInternal(v); }, title, value, tr("edit title"), true));
}

void DocumentInfo::SetSubtitleInternal(QString value)
{
	subtitle = value;
	emit SubtitleChanged(subtitle);
}

void DocumentInfo::SetSubtitle(QString value)
{
	if (value == subtitle)
		return;
	document->GetHistory()->Add(new EditValueAction<QString>([this](QString v){ SetSubtitleInternal(v); }, subtitle, value, tr("edit subtitle"), true));
}

void DocumentInfo::SetGenreInternal(QString value)
{
	genre = value;
	emit GenreChanged(genre);
}

void DocumentInfo::SetGenre(QString value){
	if (value == genre)
		return;
	document->GetHistory()->Add(new EditValueAction<QString>([this](QString v){ SetGenreInternal(v); }, genre, value, tr("edit genre"), true));
}

void DocumentInfo::SetArtistInternal(QString value)
{
	artist = value;
	emit ArtistChanged(artist);
}

void DocumentInfo::SetArtist(QString value){
	if (value == artist)
		return;
	document->GetHistory()->Add(new EditValueAction<QString>([this](QString v){ SetArtistInternal(v); }, artist, value, tr("edit artist"), true));
}

void DocumentInfo::SetSubartistsInternal(QStringList value)
{
	subartists = value;
	emit SubartistsChanged(subartists);
}

void DocumentInfo::SetSubartists(QStringList value)
{
	if (value == subartists)
		return;
	document->GetHistory()->Add(
		new EditValueAction<QStringList>(
			[this](QStringList v){ SetSubartistsInternal(v); },
			subartists, value,
			tr("edit subartists"), true));
}

void DocumentInfo::SetChartNameInternal(QString value)
{
	emit ChartNameChanged(chartName = value);
}

void DocumentInfo::SetChartName(QString value)
{
	if (value == chartName)
		return;
	document->GetHistory()->Add(
		new EditValueAction<QString>(
			[this](QString v){ SetChartNameInternal(v); },
			chartName, value,
			tr("edit chart name"), true));
}

void DocumentInfo::SetModeHintInternal(QString value)
{
	emit ModeHintChanged(modeHint = value);
}

void DocumentInfo::SetModeHint(QString value)
{
	if (value == modeHint)
		return;
	document->GetHistory()->Add(
		new EditValueAction<QString>(
			[this](QString v){ SetModeHintInternal(v); },
			modeHint, value,
			tr("edit mode hint"), true));
}

void DocumentInfo::SetJudgeRankInternal(int value)
{
	judgeRank = value;
	emit JudgeRankChanged(judgeRank);
}

void DocumentInfo::SetJudgeRank(double value){
	if (value == judgeRank)
		return;
	document->GetHistory()->Add(new EditValueAction<int>([this](double v){ SetJudgeRankInternal(v); }, judgeRank, value, tr("edit judge rank"), true));
}

void DocumentInfo::SetTotalInternal(double value)
{
	total = value;
	emit TotalChanged(total);
}

void DocumentInfo::SetTotal(double value){
	if (value == total)
		return;
	document->GetHistory()->Add(new EditValueAction<double>([this](double v){ SetTotalInternal(v); }, total, value, tr("edit total"), true));
}

void DocumentInfo::SetInitBpmInternal(double value)
{
	initBpm = value;
	emit InitBpmChanged(initBpm);
}

void DocumentInfo::SetInitBpm(double value)
{
	if (value == initBpm)
		return;
	if (!BmsConsts::IsBpmValid(value)){
		// don't change but notify current value (in order to revert value in edit)
		emit InitBpmChanged(initBpm);
		return;
	}
	document->GetHistory()->Add(new EditValueAction<double>([this](double v){ SetInitBpmInternal(v); }, initBpm, value, tr("edit initial BPM"), true));
}

void DocumentInfo::SetLevelInternal(int value)
{
	level = value;
	emit LevelChanged(level);
}

void DocumentInfo::SetLevel(int value){
	if (value == level)
		return;
	document->GetHistory()->Add(new EditValueAction<int>([this](int v){ SetLevelInternal(v); }, level, value, tr("edit level"), true));
}

void DocumentInfo::SetBackImageInternal(QString value)
{
	emit BackImageChanged(backImage = value);
}

void DocumentInfo::SetBackImage(QString value)
{
	if (value == backImage)
		return;
	document->GetHistory()->Add(
		new EditValueAction<QString>(
			[this](QString v){ SetBackImageInternal(v); },
			backImage, value,
			tr("edit back image"), true));
}

void DocumentInfo::SetEyecatchInternal(QString value)
{
	emit EyecatchImageChanged(eyecatchImage = value);
}

void DocumentInfo::SetEyecatchImage(QString value)
{
	if (value == eyecatchImage)
		return;
	document->GetHistory()->Add(
		new EditValueAction<QString>(
			[this](QString v){ SetEyecatchInternal(v); },
			eyecatchImage, value,
				tr("edit eyecatch image"), true));
}

void DocumentInfo::SetTitleImageInternal(QString value)
{
	emit TitleImageChanged(titleImage = value);
}

void DocumentInfo::SetTitleImage(QString value)
{
	if (value == titleImage)
		return;
	document->GetHistory()->Add(
		new EditValueAction<QString>(
			[this](QString v){ SetTitleImageInternal(v); },
			titleImage, value,
			tr("edit title image"), true));
}

void DocumentInfo::SetBannerInternal(QString value)
{
	emit BannerChanged(banner = value);
}

void DocumentInfo::SetBanner(QString value)
{
	if (value == banner)
		return;
	document->GetHistory()->Add(
		new EditValueAction<QString>(
			[this](QString v){ SetBannerInternal(v); },
			banner, value,
				tr("edit banner"), true));
}

void DocumentInfo::SetPreviewMusicInternal(QString value)
{
	emit PreviewMusicChanged(previewMusic = value);
}

void DocumentInfo::SetPreviewMusic(QString value)
{
	if (value == previewMusic)
		return;
	document->GetHistory()->Add(
		new EditValueAction<QString>(
			[this](QString v){ SetPreviewMusicInternal(v); },
			previewMusic, value,
			tr("edit preview music"), true));
}

QMap<QString, QJsonValue> DocumentInfo::GetExtraFields() const
{
	QMap<QString, QJsonValue> fields;
	for (QJsonObject::const_iterator i=bmsonFields.begin(); i!=bmsonFields.end(); i++){
		if (!SupportedKeys.contains(i.key())){
			fields.insert(i.key(), i.value());
		}
	}
	return fields;
}

void DocumentInfo::SetExtraFieldsInternal(QMap<QString, QJsonValue> value)
{
	for (auto i=bmsonFields.begin(); i!=bmsonFields.end(); ){
		if (!SupportedKeys.contains(i.key())){
			i = bmsonFields.erase(i);
			continue;
		}
		i++;
	}
	for (auto i=value.begin(); i!=value.end(); i++){
		bmsonFields.insert(i.key(), i.value());
	}
	emit ExtraFieldsChanged();
}

void DocumentInfo::SetExtraFields(QMap<QString, QJsonValue> fields)
{
	bool changed = false;
	QMap<QString, QJsonValue> oldBmsonFields;
	for (auto i=bmsonFields.begin(); i!=bmsonFields.end(); i++){
		if (!SupportedKeys.contains(i.key())){
			oldBmsonFields.insert(i.key(), i.value());
		}
	}
	for (auto i=fields.begin(); i!=fields.end(); ){
		if (SupportedKeys.contains(i.key())){
			i = fields.erase(i);
			continue;
		}
		i++;
	}
	changed = oldBmsonFields != fields;
	if (!changed){
		// don't add action to undo stack but notify
		emit ExtraFieldsChanged();
		return;
	}
	emit ExtraFieldsChanged();
	document->GetHistory()->Add(new EditValueAction<QMap<QString, QJsonValue>>(
								[this](QMap<QString, QJsonValue> v){ SetExtraFieldsInternal(v); },
								oldBmsonFields, fields, tr("edit extra fields"), true));
}

void DocumentInfo::ForceSetResolution(int value)
{
	resolution = value;
	emit ResolutionChanged(resolution);
}


