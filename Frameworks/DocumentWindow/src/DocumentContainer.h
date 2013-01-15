#import <document/document.h>
#import <OakTextView/OakTextView.h>
#import <OakTextView/OakDocumentView.h>

@class OakTextView;
@interface DocumentContainer : NSView <OakExtensionDelegate>
@property (nonatomic, readonly) OakTextView* textView;
@property (nonatomic, readonly) OakDocumentView* documentView;
@property (nonatomic, assign) document::document_ptr const& document;
- (void)setThemeWithUUID:(NSString*)themeUUID;
@end