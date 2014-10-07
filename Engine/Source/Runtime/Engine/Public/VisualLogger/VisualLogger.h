// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once
#include "VisualLoggerTypes.h"
#include "EngineDefines.h"

#if ENABLE_VISUAL_LOG

#define REDIRECT_TO_VLOG(Dest) FVisualLogger::Get().Redirect(this, Dest)
#define REDIRECT_OBJECT_TO_VLOG(Src, Dest) FVisualLogger::Get().Redirect(Src, Dest)

#define CONNECT_WITH_VLOG(Dest)
#define CONNECT_OBJECT_WITH_VLOG(Src, Dest)

// Text, regular log
#define UE_VLOG(LogOwner, CategoryName, Verbosity, Format, ...) FVisualLogger::CategorizedLogf(LogOwner, CategoryName, ELogVerbosity::Verbosity, INDEX_NONE, Format, ##__VA_ARGS__)
#define UE_CVLOG(Condition, LogOwner, CategoryName, Verbosity, Format, ...)  if(Condition) {UE_VLOG(LogOwner, CategoryName, Verbosity, Format, ##__VA_ARGS__);} 
// Segment shape
#define UE_VLOG_SEGMENT(LogOwner, CategoryName, Verbosity, SegmentStart, SegmentEnd, Color, Format, ...) FVisualLogger::GeometryShapeLogf(LogOwner, CategoryName, ELogVerbosity::Verbosity, INDEX_NONE, SegmentStart, SegmentEnd, Color, Format, ##__VA_ARGS__)
#define UE_CVLOG_SEGMENT(Condition, LogOwner, CategoryName, Verbosity, SegmentStart, SegmentEnd, Color, Format, ...) if(Condition) {UE_VLOG_SEGMENT(LogOwner, CategoryName, Verbosity, INDEX_NONE, SegmentStart, SegmentEnd, Color, Format, ##__VA_ARGS__);}
// Segment shape
#define UE_VLOG_SEGMENT_THICK(LogOwner, CategoryName, Verbosity, SegmentStart, SegmentEnd, Color, Thickness, Format, ...) FVisualLogger::GeometryShapeLogf(LogOwner, CategoryName, ELogVerbosity::Verbosity, INDEX_NONE, SegmentStart, SegmentEnd, Color, Format, ##__VA_ARGS__)
#define UE_CVLOG_SEGMENT_THICK(Condition, LogOwner, CategoryName, Verbosity, SegmentStart, SegmentEnd, Color, Thickness, Format, ...) if(Condition) {UE_VLOG_SEGMENT_THICK(LogOwner, CategoryName, Verbosity, INDEX_NONE, SegmentStart, SegmentEnd, Color, Format, ##__VA_ARGS__);} 
// Localization as sphere shape
#define UE_VLOG_LOCATION(LogOwner, CategoryName, Verbosity, Location, Radius, Color, Format, ...) FVisualLogger::GeometryShapeLogf(LogOwner, CategoryName, ELogVerbosity::Verbosity, INDEX_NONE, Location, Radius, Color, Format, ##__VA_ARGS__)
#define UE_CVLOG_LOCATION(Condition, LogOwner, CategoryName, Verbosity, Location, Radius, Color, Format, ...)  if(Condition) {UE_VLOG_LOCATION(LogOwner, CategoryName, Verbosity, Location, Radius, Color, Format, ##__VA_ARGS__);} 
// Box shape
#define UE_VLOG_BOX(LogOwner, CategoryName, Verbosity, Box, Color, Format, ...)  FVisualLogger::GeometryShapeLogf(LogOwner, CategoryName, ELogVerbosity::Verbosity, INDEX_NONE, Box, Color, Format, ##__VA_ARGS__)
#define UE_CVLOG_BOX(Condition, LogOwner, CategoryName, Verbosity, Box, Color, Format, ...)  if(Condition) {UE_VLOG_BOX(Actor, CategoryName, Verbosity, Box, Color, Format, ##__VA_ARGS__);} 
// Cone shape
#define UE_VLOG_CONE(LogOwner, CategoryName, Verbosity, Orgin, Direction, Length, Angle, Color, Format, ...) FVisualLogger::GeometryShapeLogf(LogOwner, CategoryName, ELogVerbosity::Verbosity, INDEX_NONE, Orgin, Direction, Length, Angle, Color, Format, ##__VA_ARGS__)
#define UE_CVLOG_CONE(Condition, LogOwner, CategoryName, Verbosity, Orgin, Direction, Length, Angle, Color, Format, ...)  if(Condition) {UE_VLOG_CONE(Actor, CategoryName, Verbosity, Orgin, Direction, Length, Angle, Color, Format, ##__VA_ARGS__);} 
// Cylinder shape
#define UE_VLOG_CYLINDER(LogOwner, CategoryName, Verbosity, Start, End, Radius, Color, Format, ...) FVisualLogger::GeometryShapeLogf(LogOwner, CategoryName, ELogVerbosity::Verbosity, INDEX_NONE, Start, End, Radius, Color, Format, ##__VA_ARGS__)
#define UE_CVLOG_CYLINDER(LogOwner, CategoryName, Verbosity, Start, End, Radius, Color, Format, ...)  if(Condition) {UE_VLOG_CYLINDER(LogOwner, CategoryName, Verbosity, Start, End, Radius, Color, Format, ##__VA_ARGS__);} 
// Capsule shape
#define UE_VLOG_CAPSULE(LogOwner, CategoryName, Verbosity, Center, HalfHeight, Radius, Rotation, Color, Format, ...) FVisualLogger::GeometryShapeLogf(LogOwner, CategoryName, ELogVerbosity::Verbosity, INDEX_NONE, Center, HalfHeight, Radius, Rotation, Color, Format, ##__VA_ARGS__)
#define UE_CVLOG_CAPSULE(LogOwner, CategoryName, Verbosity, Center, HalfHeight, Radius, Rotation, Color, Format, ...)  if(Condition) {UE_VLOG_CAPSULE(LogOwner, CategoryName, Verbosity, Center, HalfHeight, Radius, Rotation, Color, Format, ##__VA_ARGS__);} 
// Histogram data for 2d graphs 
#define UE_VLOG_HISTOGRAM(LogOwner, CategoryName, Verbosity, GraphName, DataName, Data) FVisualLogger::HistogramDataLogf(LogOwner, CategoryName, ELogVerbosity::Verbosity, INDEX_NONE, GraphName, DataName, Data, FColor::White, TEXT(""))
#define UE_CVLOG_HISTOGRAM(LogOwner, CategoryName, Verbosity, GraphName, DataName, Data) if(Condition) {UE_VLOG_HISTOGRAM(LogOwner, CategoryName, Verbosity, GraphName, DataName, Data);} 

#else
#define REDIRECT_TO_VLOG(Dest)
#define REDIRECT_OBJECT_TO_VLOG(Src, Dest)
#define CONNECT_WITH_VLOG(Dest)
#define CONNECT_OBJECT_WITH_VLOG(Src, Dest)

#define UE_VLOG(Actor, CategoryName, Verbosity, Format, ...)
#define UE_CVLOG(Condition, Actor, CategoryName, Verbosity, Format, ...)
#define UE_VLOG_SEGMENT(Actor, CategoryName, Verbosity, SegmentStart, SegmentEnd, Color, DescriptionFormat, ...)
#define UE_CVLOG_SEGMENT(Actor, CategoryName, Verbosity, SegmentStart, SegmentEnd, Color, DescriptionFormat, ...)
#define UE_VLOG_SEGMENT_THICK(Actor, CategoryName, Verbosity, SegmentStart, SegmentEnd, Color, DescriptionFormat, ...)
#define UE_CVLOG_SEGMENT_THICK(Actor, CategoryName, Verbosity, SegmentStart, SegmentEnd, Color, DescriptionFormat, ...)
#define UE_VLOG_LOCATION(Actor, CategoryName, Verbosity, Location, Radius, Color, DescriptionFormat, ...)
#define UE_CVLOG_LOCATION(Actor, CategoryName, Verbosity, Location, Radius, Color, DescriptionFormat, ...)
#define UE_VLOG_BOX(Actor, CategoryName, Verbosity, Box, Color, DescriptionFormat, ...) 
#define UE_CVLOG_BOX(Actor, CategoryName, Verbosity, Box, Color, DescriptionFormat, ...) 
#define UE_VLOG_CONE(Object, CategoryName, Verbosity, Orgin, Direction, Length, Angle, Color, DescriptionFormat, ...)
#define UE_CVLOG_CONE(Object, CategoryName, Verbosity, Orgin, Direction, Length, Angle, Color, DescriptionFormat, ...)
#define UE_VLOG_CYLINDER(Object, CategoryName, Verbosity, Start, End, Radius, Color, DescriptionFormat, ...)
#define UE_CVLOG_CYLINDER(Object, CategoryName, Verbosity, Start, End, Radius, Color, DescriptionFormat, ...)
#define UE_VLOG_CAPSULE(Object, CategoryName, Verbosity, Center, HalfHeight, Radius, Rotation, Color, DescriptionFormat, ...)
#define UE_CVLOG_CAPSULE(Object, CategoryName, Verbosity, Center, HalfHeight, Radius, Rotation, Color, DescriptionFormat, ...)
#define UE_VLOG_HISTOGRAM(Actor, CategoryName, Verbosity, GraphName, DataName, Data)
#define UE_CVLOG_HISTOGRAM(Actor, CategoryName, Verbosity, GraphName, DataName, Data)

#endif //ENABLE_VISUAL_LOG

// helper macros
#define TEXT_EMPTY TEXT("")


#if ENABLE_VISUAL_LOG

DECLARE_LOG_CATEGORY_EXTERN(LogVisual, Warning, All);

class FVisualLogDevice;
class FVisualLogExtensionInterface;

DECLARE_DELEGATE_RetVal(FString, FVisualLogFilenameGetterDelegate);

class ENGINE_API FVisualLogger : public FOutputDevice
{
public:
	// Regular text log
	VARARG_DECL(static void, static void, return, CategorizedLogf, VARARG_NONE, const TCHAR*, VARARG_EXTRA(const class UObject* LogOwner) VARARG_EXTRA(const struct FLogCategoryBase& Category) VARARG_EXTRA(ELogVerbosity::Type Verbosity) VARARG_EXTRA(int32 UniqueLogId), VARARG_EXTRA(LogOwner) VARARG_EXTRA(Category) VARARG_EXTRA(Verbosity) VARARG_EXTRA(UniqueLogId));
	// Segment log
	VARARG_DECL(static void, static void, return, GeometryShapeLogf, VARARG_NONE, const TCHAR*, VARARG_EXTRA(const class UObject* LogOwner) VARARG_EXTRA(const struct FLogCategoryBase& Category) VARARG_EXTRA(ELogVerbosity::Type Verbosity) VARARG_EXTRA(int32 UniqueLogId) VARARG_EXTRA(const FVector& Start) VARARG_EXTRA(const FVector& End) VARARG_EXTRA(const FColor& Color), VARARG_EXTRA(LogOwner) VARARG_EXTRA(Category) VARARG_EXTRA(Verbosity) VARARG_EXTRA(UniqueLogId) VARARG_EXTRA(Start) VARARG_EXTRA(End) VARARG_EXTRA(Color));
	// Location/Shpere log
	VARARG_DECL(static void, static void, return, GeometryShapeLogf, VARARG_NONE, const TCHAR*, VARARG_EXTRA(const class UObject* LogOwner) VARARG_EXTRA(const struct FLogCategoryBase& Category) VARARG_EXTRA(ELogVerbosity::Type Verbosity) VARARG_EXTRA(int32 UniqueLogId) VARARG_EXTRA(const FVector& Location) VARARG_EXTRA(float Radius) VARARG_EXTRA(const FColor& Color), VARARG_EXTRA(LogOwner) VARARG_EXTRA(Category) VARARG_EXTRA(Verbosity) VARARG_EXTRA(UniqueLogId) VARARG_EXTRA(Location) VARARG_EXTRA(Radius) VARARG_EXTRA(Color));
	// Box log
	VARARG_DECL(static void, static void, return, GeometryShapeLogf, VARARG_NONE, const TCHAR*, VARARG_EXTRA(const class UObject* LogOwner) VARARG_EXTRA(const struct FLogCategoryBase& Category) VARARG_EXTRA(ELogVerbosity::Type Verbosity) VARARG_EXTRA(int32 UniqueLogId) VARARG_EXTRA(const FBox& Box) VARARG_EXTRA(const FColor& Color), VARARG_EXTRA(LogOwner) VARARG_EXTRA(Category) VARARG_EXTRA(Verbosity) VARARG_EXTRA(UniqueLogId) VARARG_EXTRA(Box) VARARG_EXTRA(Color));
	// Cone log
	VARARG_DECL(static void, static void, return, GeometryShapeLogf, VARARG_NONE, const TCHAR*, VARARG_EXTRA(const class UObject* LogOwner) VARARG_EXTRA(const struct FLogCategoryBase& Category) VARARG_EXTRA(ELogVerbosity::Type Verbosity) VARARG_EXTRA(int32 UniqueLogId) VARARG_EXTRA(const FVector& Orgin) VARARG_EXTRA(const FVector& Direction) VARARG_EXTRA(const float Length) VARARG_EXTRA(const float Angle) VARARG_EXTRA(const FColor& Color), VARARG_EXTRA(LogOwner) VARARG_EXTRA(Category) VARARG_EXTRA(Verbosity) VARARG_EXTRA(UniqueLogId) VARARG_EXTRA(Orgin) VARARG_EXTRA(Direction) VARARG_EXTRA(Length) VARARG_EXTRA(Angle) VARARG_EXTRA(Color));
	// Cylinder log
	VARARG_DECL(static void, static void, return, GeometryShapeLogf, VARARG_NONE, const TCHAR*, VARARG_EXTRA(const class UObject* LogOwner) VARARG_EXTRA(const struct FLogCategoryBase& Category) VARARG_EXTRA(ELogVerbosity::Type Verbosity) VARARG_EXTRA(int32 UniqueLogId) VARARG_EXTRA(const FVector& Start) VARARG_EXTRA(const FVector& End) VARARG_EXTRA(const float Radius) VARARG_EXTRA(const FColor& Color), VARARG_EXTRA(LogOwner) VARARG_EXTRA(Category) VARARG_EXTRA(Verbosity) VARARG_EXTRA(UniqueLogId) VARARG_EXTRA(Start) VARARG_EXTRA(End) VARARG_EXTRA(Radius) VARARG_EXTRA(Color));
	// Capsule log
	VARARG_DECL(static void, static void, return, GeometryShapeLogf, VARARG_NONE, const TCHAR*, VARARG_EXTRA(const class UObject* LogOwner) VARARG_EXTRA(const struct FLogCategoryBase& Category) VARARG_EXTRA(ELogVerbosity::Type Verbosity) VARARG_EXTRA(int32 UniqueLogId) VARARG_EXTRA(const FVector& Center) VARARG_EXTRA(float HalfHeight) VARARG_EXTRA(float Radius) VARARG_EXTRA(const FQuat & Rotation) VARARG_EXTRA(const FColor& Color), VARARG_EXTRA(LogOwner) VARARG_EXTRA(Category) VARARG_EXTRA(Verbosity) VARARG_EXTRA(UniqueLogId) VARARG_EXTRA(Center) VARARG_EXTRA(HalfHeight) VARARG_EXTRA(Radius) VARARG_EXTRA(Rotation) VARARG_EXTRA(Color));
	//Histogram data
	VARARG_DECL(static void, static void, return, HistogramDataLogf, VARARG_NONE, const TCHAR*, VARARG_EXTRA(const class UObject* LogOwner) VARARG_EXTRA(const struct FLogCategoryBase& Category) VARARG_EXTRA(ELogVerbosity::Type Verbosity) VARARG_EXTRA(int32 UniqueLogId) VARARG_EXTRA(FName GraphName) VARARG_EXTRA(FName DataName) VARARG_EXTRA(const FVector2D& Data) VARARG_EXTRA(const FColor& Color), VARARG_EXTRA(LogOwner) VARARG_EXTRA(Category) VARARG_EXTRA(Verbosity) VARARG_EXTRA(UniqueLogId) VARARG_EXTRA(GraphName) VARARG_EXTRA(DataName) VARARG_EXTRA(Data) VARARG_EXTRA(Color));

	// static getter
	static FVisualLogger& Get()
	{
		static FVisualLogger GVisLog;
		return GVisLog;
	}

	FVisualLogger();
	virtual ~FVisualLogger();

	// Removes all logged data 
	void Cleanup(bool bReleaseMemory = false);

	/** Set log owner redirection from one object to another, to combine logs */
	static void Redirect(class UObject* FromObject, class UObject* ToObject);

	/** find and return redirection object for given object*/
	static class UObject* FindRedirection(const class UObject* Object);

	/** blocks all categories from logging. It can be bypassed with white list for categories */
	FORCEINLINE void BlockAllCategories(bool InBlock) { bBlockedAllCategories = InBlock; }

	/** checks if all categories are blocked */
	FORCEINLINE bool IsBlockedForAllCategories() { return !!bBlockedAllCategories; }

	/** Returns white list for modifications */
	TArray<class FName>& GetWhiteList() { return CategoriesWhiteList; }

	FORCEINLINE void AddCategortyToWhiteList(class FName Category) { CategoriesWhiteList.AddUnique(Category); }

	FORCEINLINE void ClearWhiteList() { CategoriesWhiteList.Reset();  }

	/** Generates and returns Id unique for given timestamp - used to connect different logs between (ex. text log with geometry shape) */
	int32 GetUniqueId(float Timestamp);

	/** Starts visual log collecting and recording */
	void SetIsRecording(bool InIsRecording);
	/** return information is vlog recording is enabled or not */
	bool IsRecording() { return !!bIsRecording; }

	/** Starts visual log collecting and recording */
	void SetIsRecordingToFile(bool InIsRecording);
	/** return information is vlog recording is enabled or not */
	bool IsRecordingToFile() { return !!bIsRecordingToFile; }

	void SetIsRecordingOnServer(bool IsRecording) { bIsRecordingOnServer = IsRecording; }
	bool IsRecordingOnServer() { return !!bIsRecordingOnServer; }

	/** Add visual logger output device */
	void AddDevice(FVisualLogDevice* InDevice) { OutputDevices.AddUnique(InDevice); }
	/** Remove visual logger output device */
	void RemoveDevice(FVisualLogDevice* InDevice) { OutputDevices.RemoveSwap(InDevice); }
	/** Remove visual logger output device */
	TArray<FVisualLogDevice*>& GetDevices() { return OutputDevices; }
	/** Returns  current entry for given TimeStap or creates another one  but first it serialize previous entry as completed to vislog devices. Use VisualLogger::DontCreate to get current entry without serialization*/
	FVisualLogEntry* GetEntryToWrite(const class UObject* Object, float TimeStamp, VisualLogger::ECreateIfNeeded ShouldCreate = VisualLogger::Create);

	/** FileName getter to set project specific file name for vlogs - highly encouradged to use FVisualLogFilenameGetterDelegate::CreateUObject with this */
	void SetLogFileNameGetter(const FVisualLogFilenameGetterDelegate& InLogFileNameGetter) { LogFileNameGetter = InLogFileNameGetter; }

	/** Register extension to use by LogVisualizer  */
	void RegisterExtension(FName TagName, FVisualLogExtensionInterface* ExtensionInterface) { check(AllExtensions.Contains(TagName) == false); AllExtensions.Add(TagName, ExtensionInterface); }
	/**  Removes previously registered extension */
	void UnregisterExtension(FName TagName, FVisualLogExtensionInterface* ExtensionInterface) { AllExtensions.Remove(TagName); }
	/** returns extension identified by given tag */
	FVisualLogExtensionInterface* GetExtensionForTag(const FName TagName) { return AllExtensions.Contains(TagName) ? AllExtensions[TagName] : NULL; }
	/** Returns reference to map with all registered extension */
	TMap<FName, FVisualLogExtensionInterface*>& GetAllExtensions() { return AllExtensions; }

private:
	virtual void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category) override { ensureMsgf(0, TEXT("Regular serialize is forbiden for visual logs")); }

protected:
	/** Array of output devices to redirect to */
	TArray<FVisualLogDevice*> OutputDevices;
	// Map for inter-objects redirections
	static TMap<class UObject*, TArray<const class UObject*> > RedirectionMap;
	// white list of categories to bypass blocking
	TArray<class FName>	CategoriesWhiteList;
	// Visual Logger extensions map
	TMap<FName, FVisualLogExtensionInterface*> AllExtensions;
	// last generated unique id for given times tamp
	TMap<float, int32> LastUniqueIds;
	// Current entry with all data
	TMap<const class UObject*, FVisualLogEntry>	CurrentEntryPerObject;
	// if set all categories are blocked from logging
	int32 bBlockedAllCategories : 1;
	// if set we are recording and collecting all vlog data
	int32 bIsRecording : 1;
	// if set we are recording to file
	int32 bIsRecordingToFile : 1;
	// variable set (from cheat manager) when logging is active on server
	int32 bIsRecordingOnServer : 1;
	// start recording time
	float StartRecordingToFileTime;
	/** Delegate to set project specific file name for vlogs */
	FVisualLogFilenameGetterDelegate LogFileNameGetter;
};

/**
 * Interface for Visual Logger Device
 */
class FVisualLogDevice
{
public:
	virtual void Cleanup(bool bReleaseMemory = false) = 0;
	virtual void StartRecordingToFile(float TImeStamp) = 0;
	virtual void StopRecordingToFile(float TImeStamp) = 0;
	virtual void Serialize(const class UObject* LogOwner, const FVisualLogEntry& LogEntry) = 0;
	virtual void SetFileName(const FString& InFileName) = 0;
	virtual bool HasFlags(int32 InFlags) { return !!(InFlags & (VisualLogger::CanSaveToFile | VisualLogger::StoreLogsLocally)); }
};

/**
 * Interface for extensions
 */
class FVisualLogExtensionInterface
{
public:
	virtual void OnTimestampChange(float Timestamp, class UWorld* InWorld, class AActor* HelperActor) = 0;
	virtual void DrawData(class UWorld* InWorld, class UCanvas* Canvas, class AActor* HelperActor, const FName& TagName, const FVisualLogEntry::FDataBlock& DataBlock, float Timestamp) = 0;
	virtual void DisableDrawingForData(class UWorld* InWorld, class UCanvas* Canvas, class AActor* HelperActor, const FName& TagName, const FVisualLogEntry::FDataBlock& DataBlock, float Timestamp) = 0;
	virtual void LogEntryLineSelectionChanged(TSharedPtr<struct FLogEntryItem> SelectedItem, int64 UserData, FName TagName) = 0;
};

#include "VisualLogger.inl"

#endif //ENABLE_VISUAL_LOG