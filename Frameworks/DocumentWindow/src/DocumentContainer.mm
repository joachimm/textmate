#import "DocumentContainer.h"
typedef std::tuple<document::document_ptr, document::document_t::splitview_t> document_key_t;
@implementation DocumentContainer
{
	std::map<document::document_t::splitview_t, OakDocumentView*> split_view_mapping;
	std::map<OakTextView*, document::document_t::splitview_t> text_split_mapping;
	NSMutableArray* viewCache;
	std::map<document_key_t, ng::editor_ptr> editors;
}

- (id)initWithFrame:(NSRect)aRect
{
	D(DBF_DocumentContainer, bug("%s\n", [NSStringFromRect(aRect) UTF8String]););
	if(self = [super initWithFrame:aRect])
	{
		_documentView = [[OakDocumentView alloc] init];
		[_documentView setTranslatesAutoresizingMaskIntoConstraints:NO];
		_documentView.textView.extensionDelegate = self;
		[self addSubview:self.documentView];
		split_view_mapping.insert(std::make_pair(_documentView.document->splitviews().at(0), _documentView));
		text_split_mapping.insert(std::make_pair(_documentView.textView, _documentView.document->splitviews().at(0)));

		viewCache = [NSMutableArray array];
	}
	return self;
}

-(OakTextView*)textView
{
	return self.documentView.textView;
}

- (void)setThemeWithUUID:(NSString*)themeUUID
{
	[self.documentView setThemeWithUUID:themeUUID];
}

- (document::document_ptr const&)document
{
	return self.documentView.document;
}

- (void)setDocument:(document::document_ptr const&)aDocument
{
	if(self.document == aDocument)
		return;

	split_view_mapping.clear();
	text_split_mapping.clear();


	split_view_mapping.insert(std::make_pair(self.documentView.document->splitviews().at(0), self.documentView));
	text_split_mapping.insert(std::make_pair(self.documentView.textView, self.documentView.document->splitviews().at(0)));

	self.documentView.document = aDocument;

	size_t size = self.document->splitviews().size();
	self.documentView.showSplitViewCloseButton = 1 < size;
	for(size_t index = 1; index < size;index++ )
	{
		document::document_t::splitview_t splitview = self.document->splitviews().at(index);
		OakDocumentView* docView = nil;
		if([viewCache count] > 0)
		{
			docView = [viewCache objectAtIndex:0];
			[viewCache removeObjectAtIndex:0];
		} else {
			docView = [[OakDocumentView alloc] init];
			docView.textView.extensionDelegate = self;
		}

		docView.showSplitViewCloseButton = index + 1 < size;
		[docView setTranslatesAutoresizingMaskIntoConstraints:NO];

		split_view_mapping.insert(std::make_pair(splitview, docView));
		text_split_mapping.insert(std::make_pair(docView.textView, splitview));

		[docView setDocument:self.document];

		[self addSubview:docView];

	}
	[self setNeedsUpdateConstraints:YES];
}
-(void)debugTextSplit
{
	NSLog(@"debug textView:%@", self.documentView.textView);
	NSLog(@"size:%zd", text_split_mapping.size() );
	iterate(i, text_split_mapping)
		NSLog(@"item %@ %zd", i->first, i->second.identifier());
}

- (ng::editor_ptr)editor:(OakTextView*)textView
{
	assert(text_split_mapping.find(textView) != text_split_mapping.end());
	if(self.document->splitviews().front() == text_split_mapping.at(textView))
		return ng::editor_for_document(self.document);

	auto editor_itr = editors.find(document_key_t(self.document, text_split_mapping.at(textView)));
	if(editor_itr == editors.end())
		editor_itr = editors.insert(std::make_pair(document_key_t(self.document, text_split_mapping.at(textView)), ng::editor_ptr(new ng::editor_t(self.document)))).first;
	return editor_itr->second;
}

- (std::string)selection:(OakTextView*)textView
{
	return std::find(self.document->splitviews().begin(), self.document->splitviews().end(), text_split_mapping.at(textView))->selection();
}

- (void)textView:(id)textView setSelection:(std::string)selection
{
	return std::find(self.document->splitviews().begin(), self.document->splitviews().end(), text_split_mapping.at(textView))->set_selection(selection);
}

// =============
// = Splitview =
// =============
- (void)splitView:(id)sender
{
	OakTextView* textView = [(NSMenuItem*)sender representedObject];
	ASSERT(text_split_mapping.find(textView) != text_split_mapping.end())
	auto split = self.document->create_splitview_after(text_split_mapping.at(textView), "to_s(NSStringFromRect([parentView.textView visibleRect]))", 100);

	split_view_mapping.at(text_split_mapping.at(textView)).showSplitViewCloseButton = YES;
	OakDocumentView* doc = [[OakDocumentView alloc] init];
	// setDocument triggers delegate callback editor, which uses mappings
	split_view_mapping.insert(std::make_pair(split, doc));
	text_split_mapping.insert(std::make_pair(doc.textView, split));
	
	doc.textView.extensionDelegate = self;
	[doc setDocument:self.document];
	[doc setTranslatesAutoresizingMaskIntoConstraints:NO];
	[self addSubview:doc];
	if(self.document->splitviews().back() != split)
		doc.showSplitViewCloseButton = YES;

	[self updateConstraints];
}

- (void)closeDocumentSplit:(id)sender
{
	OakTextView* textView = nil;
	bool after = false;
	if([sender isKindOfClass:[OakDocumentView class]])
	{
		textView = ((OakDocumentView*)sender).textView;
		after = true;
	}
	else if([sender isKindOfClass:[NSMenuItem class] ])
	{
		textView = [(NSMenuItem*)sender representedObject];
	}

	auto split = text_split_mapping.find(textView);
	ASSERT(split != text_split_mapping.end());

	auto removed = self.document->remove_splitview(split->second, after);
	auto docView = split_view_mapping.find(removed);
	ASSERT(docView != split_view_mapping.end());

	[viewCache addObject:docView->second];
	docView->second.showSplitViewCloseButton = NO; // clean reusable view
	
	[docView->second removeFromSuperview];
	text_split_mapping.erase(docView->second.textView);
	split_view_mapping.erase(removed);
	[self updateConstraints];
}

+ (BOOL)requiresConstraintBasedLayout
{
	return YES;
}

- (void)updateConstraints
{
	[self removeConstraints:[self constraints]];
	[super updateConstraints];
	// NSLayoutFormatAlignAllTop|NSLayoutFormatAlignAllBottom
	NSMutableArray* stackedViews = [NSMutableArray array];
	for(auto view : self.documentView.document->splitviews())
		[stackedViews addObject:split_view_mapping.at(view)];

	[self addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"V:|[topView]" options:0 metrics:nil views:@{ @"topView" : stackedViews[0] }]];
	for(size_t i = 0; i < [stackedViews count]-1; ++i)
	{
		[self addConstraint:[NSLayoutConstraint constraintWithItem:stackedViews[i] attribute:NSLayoutAttributeBottom relatedBy:NSLayoutRelationEqual toItem:stackedViews[i+1] attribute:NSLayoutAttributeTop multiplier:1 constant:0]];
		[self addConstraint:[NSLayoutConstraint constraintWithItem:stackedViews[i] attribute:NSLayoutAttributeHeight relatedBy:NSLayoutRelationEqual toItem:stackedViews[i+1] attribute:NSLayoutAttributeHeight multiplier:1 constant:0]];

	}
	[self addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"V:[bottomView]|" options:0 metrics:nil views:@{ @"bottomView" : stackedViews.lastObject }]];

	for(NSView* view in stackedViews)
		[self addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"H:|[view]|" options:0 metrics:nil views:NSDictionaryOfVariableBindings(view)]];

}

// ============
// = Delegate =
// ============

- (void)contextMenu:(NSMenu*)menu forView:(OakTextView*)view
{
	[menu addItem:[NSMenuItem separatorItem]];
	[menu addItemWithTitle:@"Split View" action:@selector(splitView:) keyEquivalent:@""].representedObject=view;

	auto split = self.document->splitviews().at(0);
	if(text_split_mapping.at(view) != split)
		[menu addItemWithTitle:@"Close Split View" action:@selector(closeDocumentSplit:) keyEquivalent:@""].representedObject=view;
}
@end