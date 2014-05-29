// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#include "WorldBrowserPrivatePCH.h"
#include "SNodePanel.h"
#include "GraphEditor.h"
#include "EdGraphUtilities.h"

#include "WorldTileModel.h"
#include "SWorldComposition.h"
#include "SWorldTileItem.h"
#include "SWorldLayers.h"
#include "WorldTileCollectionModel.h"

#define LOCTEXT_NAMESPACE "WorldBrowser"

//----------------------------------------------------------------
//
//
//----------------------------------------------------------------
struct FWorldZoomLevelsContainer 
	: public FZoomLevelsContainer
{
	float	GetZoomAmount(int32 InZoomLevel) const OVERRIDE
	{
		return 1.f/FMath::Square(GetNumZoomLevels() - InZoomLevel + 1)*2.f;
	}

	int32 GetNearestZoomLevel( float InZoomAmount ) const OVERRIDE
	{
		for (int32 ZoomLevelIndex=0; ZoomLevelIndex < GetNumZoomLevels(); ++ZoomLevelIndex)
		{
			if (InZoomAmount <= GetZoomAmount(ZoomLevelIndex))
			{
				return ZoomLevelIndex;
			}
		}

		return GetDefaultZoomLevel();
	}

	FText GetZoomText(int32 InZoomLevel) const OVERRIDE
	{
		return FText::AsNumber(GetZoomAmount(InZoomLevel));
	}

	int32	GetNumZoomLevels() const OVERRIDE
	{
		return 100;
	}

	int32	GetDefaultZoomLevel() const OVERRIDE
	{
		return GetNumZoomLevels() - 10;
	}

	EGraphRenderingLOD::Type GetLOD(int32 InZoomLevel) const OVERRIDE
	{
		return EGraphRenderingLOD::DefaultDetail;
	}
};

//----------------------------------------------------------------
//
//
//----------------------------------------------------------------
class SWorldCompositionGrid 
	: public SNodePanel
{
public:
	SLATE_BEGIN_ARGS(SWorldCompositionGrid) {}
		SLATE_ARGUMENT(TSharedPtr<FWorldTileCollectionModel>, InWorldModel)
	SLATE_END_ARGS()

	SWorldCompositionGrid()
		: CommandList(MakeShareable(new FUICommandList))
		, bHasScrollToRequest(false)
		, bHasScrollByRequest(false)
		, bIsFirstTickCall(true)
		, bHasNodeInteraction(true)
		, BoundsSnappingDistance(20.f)
	{

		SharedThumbnailRT = new FSlateTextureRenderTarget2DResource(
						FLinearColor::Black, 
						512, 
						512, 
						PF_B8G8R8A8, SF_Point, TA_Wrap, TA_Wrap, 0.0f
					);
		BeginInitResource(SharedThumbnailRT);
	}

	~SWorldCompositionGrid()
	{
		BeginReleaseResource(SharedThumbnailRT);
		FlushRenderingCommands();
		delete SharedThumbnailRT;
		
		WorldModel->SelectionChanged.RemoveAll(this);
		WorldModel->CollectionChanged.RemoveAll(this);

		FCoreDelegates::PreWorldOriginOffset.RemoveAll(this);
	}

	void Construct(const FArguments& InArgs)
	{
		ZoomLevels = new FWorldZoomLevelsContainer();

		SNodePanel::Construct();
	
		//// bind commands
		//const FWorldTileCommands& Commands = FWorldTileCommands::Get();
		//FUICommandList& ActionList = *CommandList;

		//ActionList.MapAction(Commands.FitToSelection,
		//	FExecuteAction::CreateSP(this, &SWorldCompositionGrid::FitToSelection_Executed),
		//	FCanExecuteAction::CreateSP(this, &SWorldCompositionGrid::AreAnyItemsSelected));

		//ActionList.MapAction(Commands.MoveLevelLeft,
		//	FExecuteAction::CreateSP(this, &SWorldCompositionGrid::MoveLevelLeft_Executed),
		//	FCanExecuteAction::CreateSP(this, &SWorldCompositionGrid::AreAnyItemsSelected));

		//ActionList.MapAction(Commands.MoveLevelRight,
		//	FExecuteAction::CreateSP(this, &SWorldCompositionGrid::MoveLevelRight_Executed),
		//	FCanExecuteAction::CreateSP(this, &SWorldCompositionGrid::AreAnyItemsSelected));

		//ActionList.MapAction(Commands.MoveLevelUp,
		//	FExecuteAction::CreateSP(this, &SWorldCompositionGrid::MoveLevelUp_Executed),
		//	FCanExecuteAction::CreateSP(this, &SWorldCompositionGrid::AreAnyItemsSelected));

		//ActionList.MapAction(Commands.MoveLevelDown,
		//	FExecuteAction::CreateSP(this, &SWorldCompositionGrid::MoveLevelDown_Executed),
		//	FCanExecuteAction::CreateSP(this, &SWorldCompositionGrid::AreAnyItemsSelected));

		//
		BounceCurve.AddCurve(0.0f, 1.0f);
		BounceCurve.Play();
		WorldModel = InArgs._InWorldModel;
		bUpdatingSelection = false;
	
		WorldModel->SelectionChanged.AddSP(this, &SWorldCompositionGrid::OnUpdateSelection);
		WorldModel->CollectionChanged.AddSP(this, &SWorldCompositionGrid::RefreshView);
		SelectionManager.OnSelectionChanged.BindSP(this, &SWorldCompositionGrid::OnSelectionChanged);

		FCoreDelegates::PreWorldOriginOffset.AddSP(this, &SWorldCompositionGrid::PreWorldOriginOffset);
	
		RefreshView();
	}
	
	/**  Add specified item to the grid view */
	void AddItem(TSharedPtr<FWorldTileModel> LevelModel)
	{
		auto NewNode = SNew(SWorldTileItem)
							.InWorldModel(WorldModel)
							.InItemModel(LevelModel)
							.ThumbnailRenderTarget(SharedThumbnailRT);
	
		AddGraphNode(NewNode);
	}
	
	/**  Remove specified item from the grid view */
	void RemoveItem(TSharedPtr<FLevelModel> LevelModel)
	{
		TSharedRef<SNode>* pItem = NodeToWidgetLookup.Find(LevelModel->GetNodeObject());
		if (pItem == NULL)
		{
			return;
		}

		Children.Remove(*pItem);
		VisibleChildren.Remove(*pItem);
		NodeToWidgetLookup.Remove(LevelModel->GetNodeObject());
	}
		
	/**  Updates all the items in the grid view */
	void RefreshView()
	{
		RemoveAllNodes();

		FLevelModelList AllLevels = WorldModel->GetAllLevels();
		for (auto It = AllLevels.CreateConstIterator(); It; ++It)
		{
			AddItem(StaticCastSharedPtr<FWorldTileModel>(*It));
		}
	}
		
	/**  SWidget interface */
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) OVERRIDE
	{
		SNodePanel::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

		// scroll to world center on first open
		if (bIsFirstTickCall)
		{
			bIsFirstTickCall = false;
			ViewOffset-= AllottedGeometry.Size*0.5f/GetZoomAmount();
		}

		FVector2D CursorPosition = FSlateApplication::Get().GetCursorPos();

		// Update cached variables
		WorldMouseLocation = CursorToWorldPosition(AllottedGeometry, CursorPosition);
		WorldMarqueeSize = Marquee.Rect.GetSize()/AllottedGeometry.Scale;
			
		// Update streaming preview data
		const bool bShowPotentiallyVisibleLevels = FSlateApplication::Get().GetModifierKeys().IsAltDown() && 
													AllottedGeometry.IsUnderLocation(CursorPosition);
	
		//WorldModel->UpdateStreamingPreview(WorldMouseLocation, bShowPotentiallyVisibleLevels);
			
		// deffered scroll and zooming requests
		if (bHasScrollToRequest || bHasScrollByRequest)
		{
			// zoom to
			if (RequestedAllowZoomIn)
			{
				RequestedAllowZoomIn = false;
				
				FVector2D SizeWithZoom = RequestedZoomArea*ZoomLevels->GetZoomAmount(ZoomLevel);
				
				if (SizeWithZoom.X >= AllottedGeometry.Size.X || 
					SizeWithZoom.Y >= AllottedGeometry.Size.Y)
				{
					// maximum zoom out by default
					ZoomLevel = ZoomLevels->GetDefaultZoomLevel();
					// expand zoom area little bit, so zooming will fit original area not so tight
					RequestedZoomArea*= 1.2f;
					// find more suitable zoom value
					for (int32 Zoom = 0; Zoom < ZoomLevels->GetDefaultZoomLevel(); ++Zoom)
					{
						SizeWithZoom = RequestedZoomArea*ZoomLevels->GetZoomAmount(Zoom);
						if (SizeWithZoom.X >= AllottedGeometry.Size.X || SizeWithZoom.Y >= AllottedGeometry.Size.Y)
						{
							ZoomLevel = Zoom;
							break;
						}
					}
				}
			}

			// scroll to
			if (bHasScrollToRequest)
			{
				bHasScrollToRequest = false;
				ViewOffset = RequestedScrollToValue - AllottedGeometry.Size*0.5f/GetZoomAmount();
			}

			// scroll by
			if (bHasScrollByRequest)
			{
				bHasScrollByRequest = false;
				ViewOffset+= RequestedScrollByValue;
			}
		}
	}
	
	/**  SWidget interface */
	virtual void OnArrangeChildren( const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const OVERRIDE
	{
		for (int32 ChildIndex=0; ChildIndex < VisibleChildren.Num(); ++ChildIndex)
		{
			const auto Child = StaticCastSharedRef<SWorldTileItem>(VisibleChildren[ChildIndex]);
			const EVisibility ChildVisibility = Child->GetVisibility();

			if (ArrangedChildren.Accepts(ChildVisibility))
			{
				FVector2D ChildPos = Child->GetPosition();
					
				ArrangedChildren.AddWidget(ChildVisibility,
					AllottedGeometry.MakeChild(Child,
					ChildPos - GetViewOffset(),
					Child->GetDesiredSize(), GetZoomAmount()
					));
			}
		}
	}
	
	/**  SWidget interface */
	virtual int32 OnPaint(const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, 
							FSlateWindowElementList& OutDrawElements, int32 LayerId, 
							const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const OVERRIDE
	{
		// First paint the background
		{
			LayerId = PaintBackground(AllottedGeometry, MyClippingRect, OutDrawElements, LayerId);
			LayerId++;
		}

		FArrangedChildren ArrangedChildren(EVisibility::Visible);
		ArrangeChildren(AllottedGeometry, ArrangedChildren);

		// Draw the child nodes

		// When drawing a marquee, need a preview of what the selection will be.
		const auto* SelectionToVisualize = &(SelectionManager.SelectedNodes);
		FGraphPanelSelectionSet SelectionPreview;
		if (Marquee.IsValid())
		{			
			ApplyMarqueeSelection(Marquee, SelectionManager.SelectedNodes, SelectionPreview);
			SelectionToVisualize = &SelectionPreview;
		}
	
		int32 NodesLayerId = LayerId;

		for (int32 ChildIndex = 0; ChildIndex < ArrangedChildren.Num(); ++ChildIndex)
		{
			FArrangedWidget& CurWidget = ArrangedChildren(ChildIndex);
			TSharedRef<SWorldTileItem> ChildNode = StaticCastSharedRef<SWorldTileItem>(CurWidget.Widget);
		
			ChildNode->bAffectedByMarquee = SelectionToVisualize->Contains(ChildNode->GetObjectBeingDisplayed());
			LayerId = CurWidget.Widget->Paint(CurWidget.Geometry, MyClippingRect, OutDrawElements, NodesLayerId, InWidgetStyle, ShouldBeEnabled(bParentEnabled));
			ChildNode->bAffectedByMarquee = false;
		}
		
		// Draw editable world bounds
		if (!WorldModel->IsSimulating())
		{
			float ScreenSpaceSize = FLevelCollectionModel::EditableAxisLength()*GetZoomAmount()*2.f;
			FVector2D PaintSize = FVector2D(ScreenSpaceSize, ScreenSpaceSize);
			FVector2D PaintPosition = GraphCoordToPanelCoord(FVector2D::ZeroVector) - (PaintSize*0.5f);
			FPaintGeometry EditableArea = AllottedGeometry.ToPaintGeometry(PaintPosition, PaintSize);
			EditableArea.DrawScale = 0.2f;
			FLinearColor PaintColor = FLinearColor::Red;
			PaintColor.A = 0.4f;

			FSlateDrawElement::MakeBox(
				OutDrawElements,
				++LayerId,
				EditableArea,
				FEditorStyle::GetBrush(TEXT("Graph.CompactNode.ShadowSelected")),
				MyClippingRect,
				ESlateDrawEffect::None,
				PaintColor
				);
		}
		
		// Draw the marquee selection rectangle
		PaintMarquee(AllottedGeometry, MyClippingRect, OutDrawElements, ++LayerId);

		// Draw the software cursor
		PaintSoftwareCursor(AllottedGeometry, MyClippingRect, OutDrawElements, ++LayerId);

		if(WorldModel->IsSimulating())
		{
			// Draw a surrounding indicator when PIE is active, to make it clear that the graph is read-only, etc...
			FSlateDrawElement::MakeBox(
				OutDrawElements,
				LayerId,
				AllottedGeometry.ToPaintGeometry(),
				FEditorStyle::GetBrush(TEXT("Graph.PlayInEditor")),
				MyClippingRect
				);

			// Draw a current camera position
			{
				const FSlateBrush* CameraImage = FEditorStyle::GetBrush(TEXT("WorldBrowser.SimulationViewPositon"));
				FVector ObserverPosition = WorldModel->GetObserverPosition();
				FVector2D ObserverPositionScreen = GraphCoordToPanelCoord(FVector2D(ObserverPosition.X, ObserverPosition.Y));

				FPaintGeometry PaintGeometry = AllottedGeometry.ToPaintGeometry(
					ObserverPositionScreen - CameraImage->ImageSize*0.5f, 
					CameraImage->ImageSize
					);
						
				FSlateDrawElement::MakeBox(
					OutDrawElements,
					++LayerId,
					PaintGeometry,
					CameraImage,
					MyClippingRect
				);
			}
		}

		LayerId = PaintScaleRuler(AllottedGeometry, MyClippingRect, OutDrawElements, LayerId);
		return LayerId;
	}
		
	/** SWidget interface */
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) OVERRIDE
	{
		const bool bIsRightMouseButtonDown = MouseEvent.IsMouseButtonDown( EKeys::RightMouseButton );
		const bool bIsLeftMouseButtonDown = MouseEvent.IsMouseButtonDown( EKeys::LeftMouseButton );

		PastePosition = PanelCoordToGraphCoord( MyGeometry.AbsoluteToLocal( MouseEvent.GetScreenSpacePosition() ) );

		if ( this->HasMouseCapture() )
		{
			const FVector2D CursorDelta = MouseEvent.GetCursorDelta();
			// Track how much the mouse moved since the mouse down.
			TotalMouseDelta += CursorDelta.Size();

			if (bIsRightMouseButtonDown)
			{
				FReply ReplyState = FReply::Handled();

				if( !CursorDelta.IsZero() )
				{
					bShowSoftwareCursor = true;
				}

				// Panning and mouse is outside of panel? Pasting should just go to the screen center.
				PastePosition = PanelCoordToGraphCoord( 0.5 * MyGeometry.Size );

				this->bIsPanning = true;
				ViewOffset -= CursorDelta / GetZoomAmount();

				return ReplyState;
			}
			else if (bIsLeftMouseButtonDown)
			{
				TSharedPtr<SNode> NodeBeingDragged = NodeUnderMousePtr.Pin();

				if ( IsEditable.Get() )
				{
					// Update the amount to pan panel
					UpdateViewOffset(MyGeometry, MouseEvent.GetScreenSpacePosition());

					const bool bCursorInDeadZone = TotalMouseDelta <= SlatePanTriggerDistance;

					if ( NodeBeingDragged.IsValid() )
					{
						if ( !bCursorInDeadZone )
						{
							// Note, NodeGrabOffset() comes from the node itself, so it's already scaled correctly.
							FVector2D AnchorNodeNewPos = PanelCoordToGraphCoord( MyGeometry.AbsoluteToLocal( MouseEvent.GetScreenSpacePosition() ) ) - NodeGrabOffset;

							// Dragging an unselected node automatically selects it.
							SelectionManager.StartDraggingNode(NodeBeingDragged->GetObjectBeingDisplayed(), MouseEvent);

							// Move all the selected nodes.
							{
								const FVector2D AnchorNodeOldPos = NodeBeingDragged->GetPosition();
								const FVector2D DeltaPos = AnchorNodeNewPos - AnchorNodeOldPos;
								if (DeltaPos.Size() > KINDA_SMALL_NUMBER)
								{
									MoveSelectedNodes(NodeBeingDragged, AnchorNodeNewPos);
								}
							}
						}

						return FReply::Handled();
					}
				}

				if ( !NodeBeingDragged.IsValid() )
				{
					// We are marquee selecting
					const FVector2D GraphMousePos = PanelCoordToGraphCoord( MyGeometry.AbsoluteToLocal( MouseEvent.GetScreenSpacePosition() ) );
					Marquee.Rect.UpdateEndPoint(GraphMousePos);

					FindNodesAffectedByMarquee( /*out*/ Marquee.AffectedNodes );
					return FReply::Handled();
				}
			}
		}

		return FReply::Unhandled();
	}

	/** @return Size of a marquee rectangle in world space */
	FVector2D GetMarqueeWorldSize() const
	{
		return WorldMarqueeSize;
	}

	/** @return Mouse cursor position in world space */
	FVector2D GetMouseWorldLocation() const
	{
		return WorldMouseLocation;
	}

protected:
	/**  Draws background for grid view */
	uint32 PaintBackground(const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, uint32 LayerId) const
	{
		FVector2D ScreenWorldOrigin = GraphCoordToPanelCoord(FVector2D(0, 0));
		FSlateRect ScreenRect = AllottedGeometry.GetRect();
	
		// World Y-axis
		if (ScreenWorldOrigin.X > ScreenRect.Left &&
			ScreenWorldOrigin.X < ScreenRect.Right)
		{
			TArray<FVector2D> LinePoints;
			LinePoints.Add(FVector2D(ScreenWorldOrigin.X, ScreenRect.Top));
			LinePoints.Add(FVector2D(ScreenWorldOrigin.X, ScreenRect.Bottom));

			FLinearColor YAxisColor = FLinearColor::Green;
			YAxisColor.A = 0.4f;
		
			FSlateDrawElement::MakeLines( 
				OutDrawElements,
				LayerId,
				AllottedGeometry.ToPaintGeometry(),
				LinePoints,
				MyClippingRect,
				ESlateDrawEffect::None,
				YAxisColor);
		}

		// World X-axis
		if (ScreenWorldOrigin.Y > ScreenRect.Top &&
			ScreenWorldOrigin.Y < ScreenRect.Bottom)
		{
			TArray<FVector2D> LinePoints;
			LinePoints.Add(FVector2D(ScreenRect.Left, ScreenWorldOrigin.Y));
			LinePoints.Add(FVector2D(ScreenRect.Right, ScreenWorldOrigin.Y));

			FLinearColor XAxisColor = FLinearColor::Red;
			XAxisColor.A = 0.4f;
		
			FSlateDrawElement::MakeLines( 
				OutDrawElements,
				LayerId,
				AllottedGeometry.ToPaintGeometry(),
				LinePoints,
				MyClippingRect,
				ESlateDrawEffect::None,
				XAxisColor);
		}

		return LayerId + 1;
	}

	/**  Draws current scale */
	uint32 PaintScaleRuler(const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, uint32 LayerId) const
	{
		const float	ScaleRulerLength = 100.f; // pixels
		TArray<FVector2D> LinePoints;
		LinePoints.Add(FVector2D::ZeroVector);
		LinePoints.Add(FVector2D::ZeroVector + FVector2D(ScaleRulerLength, 0.f));
	
		FSlateDrawElement::MakeLines( 
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToOffsetPaintGeometry(FVector2D(10, 40)),
			LinePoints,
			MyClippingRect,
			ESlateDrawEffect::None,
			FColor(200, 200, 200));

		const float UnitsInRuler = ScaleRulerLength/GetZoomAmount();// Pixels to world units
		const int32 UnitsInMeter = 100;
		const int32 UnitsInKilometer = UnitsInMeter*1000;
	
		FString RulerText;
		if (UnitsInRuler > UnitsInKilometer) // in kilometers
		{
			RulerText = FString::Printf(TEXT("%.2f km"), UnitsInRuler/UnitsInKilometer);
		}
		else // in meters
		{
			RulerText = FString::Printf(TEXT("%.2f m"), UnitsInRuler/UnitsInMeter);
		}
	
		FSlateDrawElement::MakeText(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToOffsetPaintGeometry(FVector2D(10, 27)),
			RulerText,
			FEditorStyle::GetFontStyle("NormalFont"),
			MyClippingRect,
			ESlateDrawEffect::None,
			FColor(200, 200, 200));
	
		return LayerId + 1;
	}
		
	/**  SWidget interface */
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent) OVERRIDE
	{
		if (CommandList->ProcessCommandBindings(InKeyboardEvent))
		{
			return FReply::Handled();
		}
	
		if (WorldModel->GetCommandList()->ProcessCommandBindings(InKeyboardEvent))
		{
			return FReply::Handled();
		}

		return SNodePanel::OnKeyDown(MyGeometry, InKeyboardEvent);
	}

	/**  SWidget interface */
	bool SupportsKeyboardFocus() const OVERRIDE
	{
		return true;
	}
	
	/**  SNodePanel interface */
	TSharedPtr<SWidget> OnSummonContextMenu(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	{
		if (WorldModel->IsReadOnly())
		{
			return SNullWidget::NullWidget;
		}

		FArrangedChildren ArrangedChildren(EVisibility::Visible);
		ArrangeChildren(MyGeometry, ArrangedChildren);

		const int32 NodeUnderMouseIndex = SWidget::FindChildUnderMouse( ArrangedChildren, MouseEvent );
		if (NodeUnderMouseIndex != INDEX_NONE)
		{
			// PRESSING ON A NODE!
			const FArrangedWidget& NodeGeometry = ArrangedChildren(NodeUnderMouseIndex);
			const FVector2D MousePositionInNode = NodeGeometry.Geometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
			TSharedRef<SNode> NodeWidgetUnderMouse = StaticCastSharedRef<SNode>( NodeGeometry.Widget );

			if (NodeWidgetUnderMouse->CanBeSelected(MousePositionInNode))
			{
				if (!SelectionManager.IsNodeSelected(NodeWidgetUnderMouse->GetObjectBeingDisplayed()))
				{
					SelectionManager.SelectSingleNode(NodeWidgetUnderMouse->GetObjectBeingDisplayed());
				}
			}
		}
		else
		{
			SelectionManager.ClearSelectionSet();
		}
	
		// Summon context menu
		FMenuBuilder MenuBuilder(true, WorldModel->GetCommandList());
		WorldModel->BuildGridMenu(MenuBuilder);
		TSharedPtr<SWidget> MenuWidget = MenuBuilder.MakeWidget();

		FSlateApplication::Get().PushMenu(
			AsShared(),
			MenuWidget.ToSharedRef(),
			MouseEvent.GetScreenSpacePosition(),
			FPopupTransitionEffect( FPopupTransitionEffect::ContextMenu )
			);

		return MenuWidget;
	}
	
	/**  SNodePanel interface */
	virtual void PopulateVisibleChildren(const FGeometry& AllottedGeometry) OVERRIDE
	{
		VisibleChildren.Empty();

		FSlateRect PanelRect = AllottedGeometry.GetRect();
		FVector2D ViewStartPos = PanelCoordToGraphCoord(FVector2D(PanelRect.Left, PanelRect.Top));
		FVector2D ViewEndPos = PanelCoordToGraphCoord(FVector2D(PanelRect.Right, PanelRect.Bottom));
		FSlateRect ViewRect(ViewStartPos, ViewEndPos);

		for (int32 ChildIndex=0; ChildIndex < Children.Num(); ++ChildIndex)
		{
			const auto Child = StaticCastSharedRef<SWorldTileItem>(Children[ChildIndex]);
		
			if (WorldModel->PassesAllFilters(Child->GetLevelModel()))
			{
				FSlateRect ChildRect = Child->GetItemRect();
				FVector2D ChildSize = ChildRect.GetSize();

				if (ChildSize.X > 0.f && 
					ChildSize.Y > 0.f && 
					FSlateRect::DoRectanglesIntersect(ChildRect, ViewRect))
				{
					VisibleChildren.Add(Child);
				}
			}
		}

		// Sort tiles such that smaller and selected tiles will be drawn on top of other tiles
		struct FVisibleTilesSorter
		{
			FVisibleTilesSorter(const FLevelCollectionModel& InWorldModel) : WorldModel(InWorldModel)
			{}
			bool operator()(const TSharedRef<SNodePanel::SNode>& A,
							const TSharedRef<SNodePanel::SNode>& B) const
			{
				TSharedRef<SWorldTileItem> ItemA = StaticCastSharedRef<SWorldTileItem>(A);
				TSharedRef<SWorldTileItem> ItemB = StaticCastSharedRef<SWorldTileItem>(B);
				return WorldModel.CompareLevelsZOrder(ItemA->GetLevelModel(), ItemB->GetLevelModel());
			}
			const FLevelCollectionModel& WorldModel;
		};

		VisibleChildren.Sort(FVisibleTilesSorter(*WorldModel.Get()));
	}

	/**  SNodePanel interface */
	virtual void OnBeginNodeInteraction(const TSharedRef<SNode>& InNodeToDrag, const FVector2D& GrabOffset) OVERRIDE
	{
		bHasNodeInteraction = true;
		SNodePanel::OnBeginNodeInteraction(InNodeToDrag, GrabOffset);
	}
	
	/**  SNodePanel interface */
	virtual void OnEndNodeInteraction(const TSharedRef<SNode>& InNodeDragged) OVERRIDE
	{
		const SWorldTileItem& Item = static_cast<const SWorldTileItem&>(InNodeDragged.Get());
		if (Item.IsItemEditable())
		{
			FVector2D AbsoluteDelta = Item.GetLevelModel()->GetLevelTranslationDelta();
			FIntPoint IntAbsoluteDelta = FIntPoint(AbsoluteDelta.X, AbsoluteDelta.Y);

			// Reset stored translation delta to 0
			WorldModel->UpdateTranslationDelta(WorldModel->GetSelectedLevels(), FVector2D::ZeroVector, false, 0.f);
	
			// In case we have non zero dragging delta, translate selected levels 
			if (IntAbsoluteDelta != FIntPoint::ZeroValue)
			{
				WorldModel->TranslateLevels(WorldModel->GetSelectedLevels(), IntAbsoluteDelta);
			}
		}
	
		bHasNodeInteraction = false;

		SNodePanel::OnEndNodeInteraction(InNodeDragged);
	}

	/** Handles selection changes in the grid view */
	void OnSelectionChanged(const FGraphPanelSelectionSet& SelectedNodes)
	{
		if (bUpdatingSelection)
		{
			return;
		}
	
		bUpdatingSelection = true;
		FLevelModelList SelectedLevels;
	
		for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
		{
			TSharedRef<SNode>* pWidget = NodeToWidgetLookup.Find(*NodeIt);
			if (pWidget != NULL)
			{
				TSharedRef<SWorldTileItem> Item = StaticCastSharedRef<SWorldTileItem>(*pWidget);
				SelectedLevels.Add(Item->GetLevelModel());
			}
		}
	
		WorldModel->SetSelectedLevels(SelectedLevels);
		bUpdatingSelection = false;
	}
	
	/** Handles selection changes in data source */
	void OnUpdateSelection()
	{
		if (bUpdatingSelection)
		{
			return;
		}

		bUpdatingSelection = true;

		SelectionManager.ClearSelectionSet();
		FLevelModelList SelectedLevels = WorldModel->GetSelectedLevels();
		for (auto It = SelectedLevels.CreateConstIterator(); It; ++It)
		{
			SelectionManager.SetNodeSelection((*It)->GetNodeObject(), true);
		}

		if (SelectionManager.AreAnyNodesSelected())
		{
			FVector2D MinCorner, MaxCorner;
			if (GetBoundsForNodes(true, MinCorner, MaxCorner, 0.f))
			{
				FVector2D TargetPosition = MaxCorner/2.f + MinCorner/2.f;
				RequestScrollTo(TargetPosition, MaxCorner - MinCorner);
			}
		}
		bUpdatingSelection = false;
	}

	/** Delegate callback: world origin is going to be moved. */
	void PreWorldOriginOffset(UWorld* InWorld, const FIntPoint& InSrcOrigin, const FIntPoint& InDstOrigin)
	{
		if (WorldModel->GetWorld() != InWorld)
		{
			return;
		}
				
		RequestScrollBy(-FVector2D(InDstOrigin - InSrcOrigin));
	}
	
	/** Handles new item added to data source */
	void OnNewItemAdded(TSharedPtr<FLevelModel> NewItem)
	{
		RefreshView();
	}

	/**  FitToSelection command handler */
	void FitToSelection_Executed()
	{
		FVector2D MinCorner, MaxCorner;
		if (GetBoundsForNodes(true, MinCorner, MaxCorner, 0.f))
		{
			RequestScrollTo((MaxCorner + MinCorner)*0.5f, MaxCorner - MinCorner, true);
		}
	}
		
	/**  @returns Whether any of the levels are selected */
	bool AreAnyItemsSelected() const
	{
		return SelectionManager.AreAnyNodesSelected();
	}

	/**  Requests view scroll to specified position and fit to specified area 
	 *   @param	InLocation		The location to scroll to
	 *   @param	InArea			The area to fit in view
	 *   @param	bAllowZoomIn	Is zoom in allowed during fit to area calculations
	 */
	void RequestScrollTo(FVector2D InLocation, FVector2D InArea, bool bAllowZoomIn = false)
	{
		bHasScrollToRequest = true;
		RequestedScrollToValue = InLocation;
		RequestedZoomArea = InArea;
		RequestedAllowZoomIn = bAllowZoomIn;
	}

	void RequestScrollBy(FVector2D InDelta)
	{
		bHasScrollByRequest = true;
		RequestedScrollByValue = InDelta;
	}
	
	/** Handlers for moving items using arrow keys */
	void MoveLevelLeft_Executed()
	{
		if (!bHasNodeInteraction)
		{
			WorldModel->TranslateLevels(WorldModel->GetSelectedLevels(), FIntPoint(-1, 0));
		}
	}

	void MoveLevelRight_Executed()
	{
		if (!bHasNodeInteraction)
		{
			WorldModel->TranslateLevels(WorldModel->GetSelectedLevels(), FIntPoint(+1, 0));
		}
	}

	void MoveLevelUp_Executed()
	{
		if (!bHasNodeInteraction)
		{
			WorldModel->TranslateLevels(WorldModel->GetSelectedLevels(), FIntPoint(0, -1));
		}
	}

	void MoveLevelDown_Executed()
	{
		if (!bHasNodeInteraction)
		{
			WorldModel->TranslateLevels(WorldModel->GetSelectedLevels(), FIntPoint(0, +1));
		}
	}	
	
	/** Moves selected nodes by specified offset */
	void MoveSelectedNodes(const TSharedPtr<SNode>& InNodeToDrag, FVector2D NewPosition)
	{
		auto ItemDragged = StaticCastSharedPtr<SWorldTileItem>(InNodeToDrag);
	
		if (ItemDragged->IsItemEditable())
		{
			// Current translation snapping value
			float SnappingDistanceWorld = 0.f;
			const bool bBoundsSnapping = (FSlateApplication::Get().GetModifierKeys().IsControlDown() == false);
			if (bBoundsSnapping)
			{
				SnappingDistanceWorld = BoundsSnappingDistance/GetZoomAmount();
			}
			else if (GetDefault<ULevelEditorViewportSettings>()->GridEnabled)
			{
				SnappingDistanceWorld = GEditor->GetGridSize();
			}
		
			FVector2D StartPosition = ItemDragged->GetPosition() - ItemDragged->GetLevelModel()->GetLevelTranslationDelta();
			FVector2D AbsoluteDelta = NewPosition - StartPosition;

			WorldModel->UpdateTranslationDelta(WorldModel->GetSelectedLevels(), AbsoluteDelta, bBoundsSnapping, SnappingDistanceWorld);
		}
	}

	/**  Converts cursor absolute position to the world position */
	FVector2D CursorToWorldPosition(const FGeometry& InGeometry, FVector2D InAbsoluteCursorPosition)
	{
		FVector2D ViewSpacePosition = (InAbsoluteCursorPosition - InGeometry.AbsolutePosition)/InGeometry.Scale;
		return PanelCoordToGraphCoord(ViewSpacePosition);
	}

private:
	/** Levels data list to display*/
	TSharedPtr<FWorldTileCollectionModel>	WorldModel;

	/** Geometry cache */
	mutable FVector2D						CachedAllottedGeometryScaledSize;
	/** Bouncing curve */
	FCurveSequence							BounceCurve;
	bool									bUpdatingSelection;
	TArray<FIntRect>						OccupiedCells;
	const TSharedRef<FUICommandList>		CommandList;

	bool									bHasScrollToRequest;
	bool									bHasScrollByRequest;
	FVector2D								RequestedScrollToValue;
	FVector2D								RequestedScrollByValue;
	FVector2D								RequestedZoomArea;
	bool									RequestedAllowZoomIn;

	bool									bIsFirstTickCall;
	// Is user interacting with a node now
	bool									bHasNodeInteraction;

	// Snapping distance in screen units for a tile bounds
	float									BoundsSnappingDistance;

	//
	// Mouse location in the world
	FVector2D								WorldMouseLocation;
	// Current marquee rectangle size in world units
	FVector2D								WorldMarqueeSize;

	FSlateTextureRenderTarget2DResource*	SharedThumbnailRT;
};

//----------------------------------------------------------------
//
//
//----------------------------------------------------------------
SWorldComposition::SWorldComposition()
{
}

SWorldComposition::~SWorldComposition()
{
	FWorldBrowserModule& WorldBrowserModule = FModuleManager::GetModuleChecked<FWorldBrowserModule>("WorldBrowser");
	WorldBrowserModule.OnBrowseWorld.RemoveAll(this);
	
	OnBrowseWorld(nullptr);
}

void SWorldComposition::Construct(const FArguments& InArgs)
{
	FWorldBrowserModule& WorldBrowserModule = FModuleManager::GetModuleChecked<FWorldBrowserModule>("WorldBrowser");
	WorldBrowserModule.OnBrowseWorld.AddSP(this, &SWorldComposition::OnBrowseWorld);

	ChildSlot
	[
		SAssignNew(ContentParent, SBorder)
		.BorderImage(FEditorStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
	];
	
	OnBrowseWorld(InArgs._InWorld);
}

void SWorldComposition::OnBrowseWorld(UWorld* InWorld)
{
	// Remove old world bindings
	ContentParent->SetContent(SNullWidget::NullWidget);
	LayersListWrapBox = nullptr;
	NewLayerButton = nullptr;
	NewLayerPopupWindow = nullptr;
	GridView = nullptr;
	TileWorldModel = nullptr;
			
	// Bind to a new world model in case it's a world composition
	if (InWorld && InWorld->WorldComposition)
	{
		// Get the shared world model for this world object
		FWorldBrowserModule& WorldBrowserModule = FModuleManager::GetModuleChecked<FWorldBrowserModule>("WorldBrowser");
		auto SharedWorldModel = WorldBrowserModule.SharedWorldModel(InWorld);
		
		// double check we have a tile world
		if (SharedWorldModel->IsTileWorld())
		{
			TileWorldModel = StaticCastSharedPtr<FWorldTileCollectionModel>(SharedWorldModel);
			ContentParent->SetContent(ConstructContentWidget());
			PopulateLayersList();
		}
	}
}

TSharedRef<SWidget> SWorldComposition::ConstructContentWidget()
{
	return 	
		SNew(SVerticalBox)

		// Layers list
		+SVerticalBox::Slot()
		.AutoHeight()
		[
			SAssignNew(LayersListWrapBox, SWrapBox)
			.UseAllottedWidth(true)
		]
				
		+SVerticalBox::Slot()
		.FillHeight(1)
		[
			SNew( SOverlay )

			// Grid view
			+SOverlay::Slot()
			[
				SAssignNew(GridView, SWorldCompositionGrid)
					.InWorldModel(TileWorldModel)
			]

			// Grid view top status bar
			+SOverlay::Slot()
			.VAlign(VAlign_Top)
			[
				SNew(SBorder)
				.BorderImage(FEditorStyle::GetBrush(TEXT("Graph.TitleBackground")))
				[
					SNew(SVerticalBox)

					+SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SHorizontalBox)
							
						// Current world view scale
						+SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNullWidget::NullWidget
						]
						+SHorizontalBox::Slot()
						.Padding(5,0,0,0)
						[
							SNullWidget::NullWidget
						]
							
						// World origin position
						+SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SImage)
							.Image(FEditorStyle::GetBrush( "WorldBrowser.WorldOrigin" ))
						]
						+SHorizontalBox::Slot()
						.Padding(5,0,0,0)
						[
							SNew(STextBlock)
							.TextStyle( FEditorStyle::Get(), "WorldBrowser.StatusBarText" )
							.Text(this, &SWorldComposition::GetCurrentOriginText)
						]

						// Current level
						+SHorizontalBox::Slot()
						.HAlign(HAlign_Right)
						.Padding(0,0,5,0)
						[
							SNew(STextBlock)
							.TextStyle( FEditorStyle::Get(), "WorldBrowser.StatusBarText" )
							.Text(this, &SWorldComposition::GetCurrentLevelText)
						]											
					]
				]
			]
			// Grid view bottom status bar
			+SOverlay::Slot()
			.VAlign(VAlign_Bottom)
			[
				SNew(SBorder)
				.BorderImage(FEditorStyle::GetBrush(TEXT("Graph.TitleBackground")))
				[
					SNew(SVerticalBox)

					+SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SHorizontalBox)

						// Mouse location
						+SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SImage)
							.Image(FEditorStyle::GetBrush( "WorldBrowser.MouseLocation" ))
						]
						+SHorizontalBox::Slot()
						.Padding(5,0,0,0)
						[
							SNew(STextBlock)
							.TextStyle( FEditorStyle::Get(), "WorldBrowser.StatusBarText" )
							.Text(this, &SWorldComposition::GetMouseLocationText)
						]

						// Selection marquee rectangle size
						+SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SImage)
							.Image(FEditorStyle::GetBrush( "WorldBrowser.MarqueeRectSize" ))
						]
						+SHorizontalBox::Slot()
						.Padding(5,0,0,0)
						[
							SNew(STextBlock)
							.TextStyle( FEditorStyle::Get(), "WorldBrowser.StatusBarText" )
							.Text(this, &SWorldComposition::GetMarqueeSelectionSizeText)
						]

						// World size
						+SHorizontalBox::Slot()
						.HAlign(HAlign_Right)
						[
							SNew(SHorizontalBox)

							+SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(SImage)
								.Image(FEditorStyle::GetBrush( "WorldBrowser.WorldSize" ))
							]

							+SHorizontalBox::Slot()
							.Padding(5,0,5,0)
							[
								SNew(STextBlock)
								.TextStyle( FEditorStyle::Get(), "WorldBrowser.StatusBarText" )
								.Text(this, &SWorldComposition::GetWorldSizeText)
							]
						]											
					]
				]
			]

			// Top-right corner text indicating that simulation is active
			+SOverlay::Slot()
			.Padding(20)
			.VAlign(VAlign_Top)
			.HAlign(HAlign_Right)
			[
				SNew(STextBlock)
				.Visibility(this, &SWorldComposition::IsSimulationVisible)
				.TextStyle( FEditorStyle::Get(), "Graph.SimulatingText" )
				.Text(FString(TEXT("SIMULATING")))
			]
		];
}

void SWorldComposition::PopulateLayersList()
{
	TArray<FWorldTileLayer>& AllLayers = TileWorldModel->GetLayers();
	
	LayersListWrapBox->ClearChildren();
	for (auto WorldLayer : AllLayers)
	{
		LayersListWrapBox->AddSlot()
		.Padding(1,1,0,0)
		[
			SNew(SWorldLayerButton)
				.InWorldModel(TileWorldModel)
				.WorldLayer(WorldLayer)
		];
	}

	// Add new layer button
	LayersListWrapBox->AddSlot()
	.Padding(1,1,0,0)
	[
		SAssignNew(NewLayerButton, SButton)
		.OnClicked(this, &SWorldComposition::NewLayer_Clicked)
		.ButtonColorAndOpacity(FLinearColor(0.2f, 0.2f, 0.2f, 0.2f))
		.Content()
		[
			SNew(SImage)
			.Image(FEditorStyle::GetBrush("WorldBrowser.AddLayer"))
		]
	];
}

FReply SWorldComposition::NewLayer_Clicked()
{
	if (TileWorldModel->IsReadOnly())
	{
		return FReply::Handled();
	}
	
	TSharedRef<SNewLayerPopup> CreateLayerWidget = 
		SNew(SNewLayerPopup)
		.OnCreateLayer(this, &SWorldComposition::CreateNewLayer)
		.DefaultName(LOCTEXT("Layer_DefaultName", "MyLayer").ToString());

	NewLayerPopupWindow = FSlateApplication::Get().PushMenu(
		this->AsShared(),
		CreateLayerWidget,
		FSlateApplication::Get().GetCursorPos(),
		FPopupTransitionEffect( FPopupTransitionEffect::TypeInPopup )
		);

	return FReply::Handled();
}

FReply SWorldComposition::CreateNewLayer(const FWorldTileLayer& NewLayer)
{
	TileWorldModel->AddManagedLayer(NewLayer);
	PopulateLayersList();
	
	if (NewLayerPopupWindow.IsValid())
	{
		NewLayerPopupWindow->RequestDestroyWindow();
	}
		
	return FReply::Handled();
}

FText SWorldComposition::GetZoomText() const
{
	return GridView->GetZoomText();
}

FString SWorldComposition::GetCurrentOriginText() const
{
	UWorld* CurrentWorld = (TileWorldModel->IsSimulating() ? TileWorldModel->GetSimulationWorld() : TileWorldModel->GetWorld());
	return FString::Printf(TEXT("%d, %d"), CurrentWorld->GlobalOriginOffset.X, CurrentWorld->GlobalOriginOffset.Y);
}

FString SWorldComposition::GetCurrentLevelText() const
{
	FString Text("");
	UWorld* CurrentWorld = (TileWorldModel->IsSimulating() ? TileWorldModel->GetSimulationWorld() : TileWorldModel->GetWorld());

	if (CurrentWorld->GetCurrentLevel())
	{
		UPackage* Package = CastChecked<UPackage>(CurrentWorld->GetCurrentLevel()->GetOutermost());
		Text+= FPackageName::GetShortName(Package->GetName());
	}
	else
	{
		Text+= TEXT("None");
	}
	
	return Text;
}

FString SWorldComposition::GetMouseLocationText() const
{
	FVector2D MouseLocation = GridView->GetMouseWorldLocation();
	return FString::Printf(TEXT("%d, %d"), FMath::RoundToInt(MouseLocation.X), FMath::RoundToInt(MouseLocation.Y));
}

FString	SWorldComposition::GetMarqueeSelectionSizeText() const
{
	FVector2D MarqueeSize = GridView->GetMarqueeWorldSize();
	
	if (MarqueeSize.X > 0 && 
		MarqueeSize.Y > 0)
	{
		return FString::Printf(TEXT("%d x %d"), FMath::RoundToInt(MarqueeSize.X), FMath::RoundToInt(MarqueeSize.Y));
	}
	else
	{
		return FString();
	}
}

FString SWorldComposition::GetWorldSizeText() const
{
	FIntPoint WorldSize = TileWorldModel->GetWorldSize();
	
	if (WorldSize.X > 0 && 
		WorldSize.Y > 0)
	{
		return FString::Printf(TEXT("%d x %d"), WorldSize.X, WorldSize.Y);
	}
	else
	{
		return FString();
	}
}

EVisibility SWorldComposition::IsSimulationVisible() const
{
	return (TileWorldModel->IsSimulating() ? EVisibility::Visible : EVisibility::Hidden);
}

#undef LOCTEXT_NAMESPACE