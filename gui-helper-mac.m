// Cocoa implementation in Objective-C, C-only gimmick not worth it for Apple
// https://developer.apple.com/documentation/appkit?language=objc

#include "gui-helper.h"
#include <stdlib.h>
#include <string.h>
#include <Cocoa/Cocoa.h>

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
    int result;
    BOOL done;
}

@property (nonatomic, strong) NSWindow* window;
@property (nonatomic, weak) NSSegmentedControl* segmentedControl;
@property (nonatomic, weak) NSScrollView* nonSteamScrollView;
@property (nonatomic, weak) NSScrollView* modsScrollView;
@property (nonatomic, weak) NSTableView* nonSteamTable;
@property (nonatomic, weak) NSTableView* modsTable;
@property (nonatomic, weak) NSButton* okButton;

- (id)initWithTitle:(const char*)title
      nonSteamItems:(const char**)nsItems
      nonSteamCount:(int)nsCount
          modsItems:(const char**)mItems
          modsCount:(int)mCount
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

    // Segmented control switches between two lists
    NSSegmentedControl* segmentedControl = [
        NSSegmentedControl segmentedControlWithLabels:@[@"Non-Steam", @"GoldSrc/Source Mods"]
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
    if (initialTab != 0) {
        initialTab = 1;
    }
    [segmentedControl setSelectedSegment:initialTab];
    [nonSteamScroll setHidden:(initialTab != 0)];
    [modsScroll setHidden:(initialTab != 1)];

    NSTableView* initialTable = (initialTab == 0) ? nonSteamTable : modsTable;
    int initialCount = (initialTab == 0) ? nonSteamCount : modsCount;
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

- (void)segmentChanged:(id)sender
{
    NSInteger selected = [self.segmentedControl selectedSegment];
    [self.nonSteamScrollView setHidden:(selected != 0)];
    [self.modsScrollView setHidden:(selected != 1)];

    NSTableView* activeTable = (selected == 0) ? self.nonSteamTable : self.modsTable;
    [self.okButton setEnabled:([activeTable selectedRow] != -1)];
}

- (void)okClicked:(id)sender
{
    NSInteger tabIndex = [self.segmentedControl selectedSegment];
    NSTableView* activeTable = (tabIndex == 0) ? self.nonSteamTable : self.modsTable;

    NSInteger row = [activeTable selectedRow];
    if (row < 0) {
        return; // nothing selected; OK should be disabled in this case anyway
    }

    result = (int)row;
    if (tabIndex != 0) {
        result += nonSteamCount;
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
    return ([tableView tag] == 0) ? nonSteamCount : modsCount;
}

- (id)tableView:(NSTableView*)tableView objectValueForTableColumn:(NSTableColumn*)tableColumn row:(NSInteger)row
{
    const char* str = ([tableView tag] == 0) ? nonSteamItems[row] : modsItems[row];
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

int SelectionDialog(const char* title, int count, const char** list, int modsCount, const char** modsList, int selection)
{
    int returnValue = -1;

    @autoreleasepool {
        [NSApplication sharedApplication];

        int initialTab = 0;
        int initialSelection = selection;
        if (selection >= count) {
            initialTab = 1;
            initialSelection = selection - count;
        }

        SGDBSelectionController* controller = [[SGDBSelectionController alloc]
            initWithTitle:title
            nonSteamItems:list
            nonSteamCount:count
                modsItems:modsList
                modsCount:modsCount
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