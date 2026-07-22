// Cocoa implementation in Objective-C, C-only gimmick not worth it for Apple
// https://developer.apple.com/documentation/appkit?language=objc

#include "gui-helper.h"
#include <stdlib.h>
#include <string.h>
#include <Cocoa/Cocoa.h>
#include <CoreServices/CoreServices.h>

static NSString *NSStringFromCString(const char *str)
{
    return [NSString stringWithUTF8String:(str != NULL) ? str : ""];
}

// https://developer.apple.com/documentation/appkit/nsalert?language=objc
int ShowMessageBox(const char *title, const char *message)
{
    [NSApplication sharedApplication];
    [NSApp activateIgnoringOtherApps:YES];

    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText:NSStringFromCString(title)];
    [alert setInformativeText:NSStringFromCString(message)];
    [alert addButtonWithTitle:@"OK"];

    NSInteger response = [alert runModal];

    return (int)response;
}

@interface SGDBSelectionController : NSObject <NSTableViewDataSource, NSTableViewDelegate, NSWindowDelegate>
{
@public
    const char** nonSteamItems;
    int nonSteamCount;
    const char** modsItems;
    int modsCount;
    const char** steamItems;
    int steamCount;
    int result;
    BOOL done;
}

@property (nonatomic, strong) NSWindow* window;
@property (nonatomic, weak) NSSegmentedControl* segmentedControl;
@property (nonatomic, weak) NSScrollView* nonSteamScrollView;
@property (nonatomic, weak) NSScrollView* modsScrollView;
@property (nonatomic, weak) NSScrollView* steamScrollView;
@property (nonatomic, weak) NSTableView* nonSteamTable;
@property (nonatomic, weak) NSTableView* modsTable;
@property (nonatomic, weak) NSTableView* steamTable;
@property (nonatomic, weak) NSButton* okButton;

- (id)initWithTitle:(const char*)title
      nonSteamItems:(const char**)nsItems
      nonSteamCount:(int)nsCount
          modsItems:(const char**)mItems
          modsCount:(int)mCount
         steamItems:(const char**)sItems
         steamCount:(int)sCount
         initialTab:(int)initialTab
   initialSelection:(int)initialSelection;

- (NSScrollView*)buildScrollingTableWithTag:(NSInteger)tag outTableView:(NSTableView**)outTableView;
- (void)segmentChanged:(id)sender;
- (void)okClicked:(id)sender;
- (void)cancelClicked:(id)sender;

@end

@implementation SGDBSelectionController

- (id)initWithTitle:(const char*)title
      nonSteamItems:(const char**)nsItems
      nonSteamCount:(int)nsCount
          modsItems:(const char**)mItems
          modsCount:(int)mCount
         steamItems:(const char**)sItems
         steamCount:(int)sCount
         initialTab:(int)initialTab
   initialSelection:(int)initialSelection
{
    self = [super init];
    if (!self) {
        return nil;
    }

    nonSteamItems = nsItems;
    nonSteamCount = nsCount;
    modsItems = mItems;
    modsCount = mCount;
    steamItems = sItems;
    steamCount = sCount;
    result = -1;
    done = NO;

    const NSRect contentRect = NSMakeRect(0, 0, 454, 388);
    const NSUInteger styleMask = NSWindowStyleMaskTitled
        | NSWindowStyleMaskClosable
        | NSWindowStyleMaskMiniaturizable
        | NSWindowStyleMaskResizable;

    NSWindow* window = [[NSWindow alloc] initWithContentRect:contentRect
                                                    styleMask:styleMask
                                                      backing:NSBackingStoreBuffered
                                                        defer:NO];
    [window setTitle:NSStringFromCString(title)];
    [window setDelegate:self];
    [window center];
    self.window = window;

    NSView* contentView = [window contentView];

    // Segmented control switches between three lists
    NSSegmentedControl* segmentedControl = [
        NSSegmentedControl segmentedControlWithLabels:@[@"Non-Steam", @"GoldSrc/Source Mods", @"Steam"]
        trackingMode:NSSegmentSwitchTrackingSelectOne
        target:self
        action:@selector(segmentChanged:)
    ];
    [segmentedControl setFrame:NSMakeRect(10, 354, 434, 24)];
    [segmentedControl setSegmentStyle:NSSegmentStyleAutomatic];
    [segmentedControl setAutoresizingMask:(NSViewWidthSizable | NSViewMinYMargin)];
    self.segmentedControl = segmentedControl;
    [contentView addSubview:segmentedControl];

    NSTableView* nonSteamTable = nil;
    NSScrollView* nonSteamScroll = [self buildScrollingTableWithTag:0 outTableView:&nonSteamTable];
    [nonSteamScroll setFrame:NSMakeRect(10, 50, 434, 294)];
    [nonSteamScroll setAutoresizingMask:(NSViewWidthSizable | NSViewHeightSizable)];
    self.nonSteamTable = nonSteamTable;
    self.nonSteamScrollView = nonSteamScroll;
    [contentView addSubview:nonSteamScroll];

    NSTableView* modsTable = nil;
    NSScrollView* modsScroll = [self buildScrollingTableWithTag:1 outTableView:&modsTable];
    [modsScroll setFrame:NSMakeRect(10, 50, 434, 294)];
    [modsScroll setAutoresizingMask:(NSViewWidthSizable | NSViewHeightSizable)];
    self.modsTable = modsTable;
    self.modsScrollView = modsScroll;
    [contentView addSubview:modsScroll];

    NSTableView* steamTable = nil;
    NSScrollView* steamScroll = [self buildScrollingTableWithTag:2 outTableView:&steamTable];
    [steamScroll setFrame:NSMakeRect(10, 50, 434, 294)];
    [steamScroll setAutoresizingMask:(NSViewWidthSizable | NSViewHeightSizable)];
    self.steamTable = steamTable;
    self.steamScrollView = steamScroll;
    [contentView addSubview:steamScroll];

    // buttons, on mac the standard is [Cancel] [OK]
    NSButton* okButton = [[NSButton alloc] initWithFrame:NSMakeRect(454 - 80 - 10, 10, 80, 30)];
    [okButton setTitle:@"OK"];
    [okButton setBezelStyle:NSBezelStyleAutomatic];
    [okButton setAutoresizingMask:(NSViewMinXMargin | NSViewMaxYMargin)];
    [okButton setTarget:self];
    [okButton setAction:@selector(okClicked:)];
    [okButton setKeyEquivalent:@"\r"];
    [contentView addSubview:okButton];
    self.okButton = okButton;

    NSButton* cancelButton = [[NSButton alloc] initWithFrame:NSMakeRect(454 - 80 - 10 - 80 - 10, 10, 80, 30)];
    [cancelButton setTitle:@"Cancel"];
    [cancelButton setBezelStyle:NSBezelStyleAutomatic];
    [cancelButton setAutoresizingMask:(NSViewMinXMargin | NSViewMaxYMargin)];
    [cancelButton setTarget:self];
    [cancelButton setAction:@selector(cancelClicked:)];
    [cancelButton setKeyEquivalent:@"\e"]; // Escape key
    [contentView addSubview:cancelButton];

    // apply initial tab + selection
    if (initialTab < 0 || initialTab > 2) {
        initialTab = 0;
    }
    [segmentedControl setSelectedSegment:initialTab];
    [nonSteamScroll setHidden:(initialTab != 0)];
    [modsScroll setHidden:(initialTab != 1)];
    [steamScroll setHidden:(initialTab != 2)];

    NSTableView* initialTable = nonSteamTable;
    int initialCount = nonSteamCount;
    if (initialTab == 1) {
        initialTable = modsTable;
        initialCount = modsCount;
    } else if (initialTab == 2) {
        initialTable = steamTable;
        initialCount = steamCount;
    }
    if (initialSelection >= 0 && initialSelection < initialCount) {
        [initialTable selectRowIndexes:[NSIndexSet indexSetWithIndex:initialSelection] byExtendingSelection:NO];
        [initialTable scrollRowToVisible:initialSelection];
        [self.okButton setEnabled:YES];
    } else {
        [self.okButton setEnabled:NO];
    }

    return self;
}

// Builds a headerless, single-column table view inside a scroll view.
- (NSScrollView*)buildScrollingTableWithTag:(NSInteger)tag outTableView:(NSTableView**)outTableView
{
    NSTableColumn* column = [[NSTableColumn alloc] initWithIdentifier:@"item"];
    [column setWidth:400];
    [column setEditable:NO];

    NSTableView* tableView = [[NSTableView alloc] initWithFrame:NSMakeRect(0, 0, 434, 328)];
    [tableView addTableColumn:column];
    [tableView setHeaderView:nil];
    [tableView setAllowsMultipleSelection:NO];
    [tableView setAllowsEmptySelection:YES];
    [tableView setTag:tag];
    [tableView setDataSource:self];
    [tableView setDelegate:self];
    [tableView setTarget:self];
    [tableView setDoubleAction:@selector(okClicked:)];

    NSScrollView* scrollView = [[NSScrollView alloc] initWithFrame:NSMakeRect(0, 0, 434, 328)];
    [scrollView setDocumentView:tableView];
    [scrollView setHasVerticalScroller:YES];
    [scrollView setAutohidesScrollers:YES];
    [scrollView setAutoresizingMask:(NSViewWidthSizable | NSViewHeightSizable)];

    if (outTableView != NULL) {
        *outTableView = tableView;
    }
    return scrollView;
}

- (NSTableView*)tableViewForSegment:(NSInteger)segment
{
    switch (segment) {
        case 0: return self.nonSteamTable;
        case 1: return self.modsTable;
        case 2: return self.steamTable;
        default: return nil;
    }
}

- (void)segmentChanged:(id)sender
{
    NSInteger selected = [self.segmentedControl selectedSegment];
    [self.nonSteamScrollView setHidden:(selected != 0)];
    [self.modsScrollView setHidden:(selected != 1)];
    [self.steamScrollView setHidden:(selected != 2)];

    NSTableView* activeTable = [self tableViewForSegment:selected];
    [self.okButton setEnabled:([activeTable selectedRow] != -1)];
}

- (void)okClicked:(id)sender
{
    NSInteger tabIndex = [self.segmentedControl selectedSegment];
    NSTableView* activeTable = [self tableViewForSegment:tabIndex];

    NSInteger row = [activeTable selectedRow];
    if (row < 0) {
        return; // nothing selected; OK should be disabled in this case anyway
    }

    result = (int)row;
    if (tabIndex == 1) {
        result += nonSteamCount;
    } else if (tabIndex == 2) {
        result += nonSteamCount + modsCount;
    }

    done = YES;
    [NSApp stopModal];
}

- (void)cancelClicked:(id)sender
{
    result = -1;
    done = YES;
    [NSApp stopModal];
}

// NSTableViewDataSource

- (NSInteger)numberOfRowsInTableView:(NSTableView*)tableView
{
    switch ([tableView tag]) {
        case 0: return nonSteamCount;
        case 1: return modsCount;
        case 2: return steamCount;
        default: return 0;
    }
}

- (id)tableView:(NSTableView*)tableView objectValueForTableColumn:(NSTableColumn*)tableColumn row:(NSInteger)row
{
    const char* str = NULL;
    switch ([tableView tag]) {
        case 0: str = nonSteamItems[row]; break;
        case 1: str = modsItems[row]; break;
        case 2: str = steamItems[row]; break;
        default: break;
    }
    return NSStringFromCString(str);
}

// NSTableViewDelegate

- (void)tableViewSelectionDidChange:(NSNotification*)notification
{
    NSTableView* tableView = [notification object];
    [self.okButton setEnabled:([tableView selectedRow] != -1)];
}

// NSWindowDelegate - clicking the red close button counts as Cancel.

- (void)windowWillClose:(NSNotification*)notification
{
    if (!done) {
        result = -1;
        done = YES;
    }
    if ([NSApp modalWindow] == self.window) {
        [NSApp stopModal];
    }
}

@end

int SelectionDialog(const char* title, int nonSteamCount, const char** nonSteamList, int modsCount, const char** modsList, int steamCount, const char** steamList, int selection)
{
    int returnValue = -1;

    @autoreleasepool {
        [NSApplication sharedApplication];

        int initialTab = 0;
        int initialSelection = selection;
        if (selection >= nonSteamCount + modsCount) {
            initialTab = 2;
            initialSelection -= nonSteamCount + modsCount;
        } else if (selection >= nonSteamCount) {
            initialTab = 1;
            initialSelection -= nonSteamCount;
        }

        SGDBSelectionController* controller = [[SGDBSelectionController alloc]
            initWithTitle:title
            nonSteamItems:nonSteamList
            nonSteamCount:nonSteamCount
                modsItems:modsList
                modsCount:modsCount
               steamItems:steamList
               steamCount:steamCount
               initialTab:initialTab
             initialSelection:initialSelection];

        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
        [NSApp activateIgnoringOtherApps:YES];
        [controller.window makeKeyAndOrderFront:nil];
        [NSApp runModalForWindow:controller.window];

        returnValue = controller->result;

        [controller.window orderOut:nil];
    }

    return returnValue;
}

static void stopRunLoop(void)
{
    [NSApp stop:nil];
    NSEvent* dummy = [NSEvent otherEventWithType:NSEventTypeApplicationDefined location:NSZeroPoint modifierFlags:0 timestamp:0 windowNumber:0 context:nil subtype:0 data1:0 data2:0];
    [NSApp postEvent:dummy atStart:YES];
}

@interface SGDBAppDelegate : NSObject <NSApplicationDelegate>
@property (nonatomic, copy) NSString* URL;
@end

@implementation SGDBAppDelegate

- (void)application:(NSApplication *)application openURLs:(NSArray<NSURL *> *)urls
{
    if (urls.count > 0) {
        self.URL = [urls[0] absoluteString];
        stopRunLoop();
    }
}

@end

const char* macAwaitEvent()
{
    const double timeout = 1.5;
    NSApplication* app = [NSApplication sharedApplication];

    SGDBAppDelegate* delegate = [[SGDBAppDelegate alloc] init];
    app.delegate = delegate;

    NSTimer* timeoutTimer = [NSTimer scheduledTimerWithTimeInterval:timeout repeats:NO block:^(NSTimer* t) {
        (void)t;
        stopRunLoop();
    }];

    [app run];

    [timeoutTimer invalidate];
    app.delegate = nil;

    static char url[512];
    url[0] = '\0';
    if (delegate.URL) {
        strncpy(url, [delegate.URL UTF8String], sizeof(url) - 1);
        url[sizeof(url) - 1] = '\0';
        return url;
    }

    return NULL;
}

// Set the URL by using the old depricated API cause the new method is annoying to implement
// https://developer.apple.com/documentation/coreservices/1447760-lssetdefaulthandlerforurlscheme?language=objc
int macSetURLHandler()
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    OSStatus status = LSSetDefaultHandlerForURLScheme(CFSTR("sgdb"), CFSTR("com.steamgriddb.SGDBoop"));
    if (status == noErr) {
        CFStringRef currentHandler = LSCopyDefaultHandlerForURLScheme(CFSTR("sgdb"));
        BOOL alreadyRegistered = (currentHandler != NULL && CFStringCompare(currentHandler, CFSTR("com.steamgriddb.SGDBoop"), 0) == kCFCompareEqualTo);
        if (currentHandler != NULL) {
            CFRelease(currentHandler);
        }

        if (alreadyRegistered) {
            ShowMessageBox("SGDBoop Information", "SGDBoop is already registered!\nHead over to https://www.steamgriddb.com/boop to continue setup.");
        } else {
            ShowMessageBox("SGDBoop Information", "Program registered successfully!\n\nSGDBoop is meant to be ran from a browser!\nHead over to https://www.steamgriddb.com/boop to continue setup.");
        }
    }
    return 0;
#pragma clang diagnostic pop
}