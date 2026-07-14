#import "gui-helper.h"
#import <Cocoa/Cocoa.h>
#import <objc/runtime.h>

// ---------------------------------------------------------------------------
// macosReceiveURLEvent — receive the sgdb:// URL delivered as an Apple Event
//
// When macOS launches the .app bundle as a URL scheme handler it sends the
// URL via a kAEGetURL Apple Event, NOT as argv[1].  To receive Apple Events:
//   1. [NSApplication sharedApplication] MUST be called first — it sets up
//      the Mach port that the OS delivers Apple Events through.
//   2. NSAppleEventManager (not the raw Carbon AEInstallEventHandler) must be
//      used, because it hooks into the NSRunLoop machinery.
//   3. The NSRunLoop (not just CFRunLoop) must be pumped to dispatch the event.
//
// Returns a pointer to a static buffer containing the URL, or NULL on timeout.
// ---------------------------------------------------------------------------

// App delegate that implements the standard URL opening protocol
@interface SGDBAppDelegate : NSObject <NSApplicationDelegate>
@property (nonatomic, copy) NSString* receivedURL;
@end

@implementation SGDBAppDelegate
- (void)dealloc
{
    [_receivedURL release];
    [super dealloc];
}

- (void)application:(NSApplication *)application openURLs:(NSArray<NSURL *> *)urls
{
    (void)application;
    if (urls.count > 0) {
        self.receivedURL = [[urls[0] absoluteString] stringByRemovingPercentEncoding];

        // Stop the AppKit event loop now that we have the URL
        [NSApp stop:nil];
        NSEvent* dummy = [NSEvent otherEventWithType:NSEventTypeApplicationDefined
                                            location:NSZeroPoint
                                       modifierFlags:0
                                           timestamp:0
                                        windowNumber:0
                                             context:nil
                                             subtype:0
                                               data1:0
                                               data2:0];
        [NSApp postEvent:dummy atStart:YES];
    }
}
@end

const char* macosReceiveURLEvent(double timeoutSeconds)
{
    static char urlBuf[600];
    urlBuf[0] = '\0';

    // Step 1: Initialize NSApplication
    NSApplication* app = [NSApplication sharedApplication];

    // Step 2: Set application delegate
    SGDBAppDelegate* delegate = [[SGDBAppDelegate alloc] init];
    app.delegate = delegate;

    // Step 3: Schedule a timeout timer to stop the event loop
    NSTimer* timeoutTimer = [NSTimer scheduledTimerWithTimeInterval:timeoutSeconds
                                                             repeats:NO
                                                               block:^(NSTimer* t) {
        (void)t;
        [NSApp stop:nil];
        NSEvent* dummy = [NSEvent otherEventWithType:NSEventTypeApplicationDefined
                                            location:NSZeroPoint
                                       modifierFlags:0
                                           timestamp:0
                                        windowNumber:0
                                             context:nil
                                             subtype:0
                                               data1:0
                                               data2:0];
        [NSApp postEvent:dummy atStart:YES];
    }];

    // Step 4: Run the AppKit event loop to receive the URL delegate event
    [app run];

    // Step 5: Clean up
    [timeoutTimer invalidate];
    app.delegate = nil;

    if (delegate.receivedURL) {
        strncpy(urlBuf, [delegate.receivedURL UTF8String], sizeof(urlBuf) - 1);
        urlBuf[sizeof(urlBuf) - 1] = '\0';
        return urlBuf;
    }

    return NULL;
}

// ---------------------------------------------------------------------------
// macosRegisterURLHandler — set SGDBoop as the default sgdb:// handler
// ---------------------------------------------------------------------------
void macosRegisterURLHandler(void)
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    // LSSetDefaultHandlerForURLScheme is deprecated in macOS 12 but is still
    // the simplest programmatic way to set the default handler without
    // requiring a full SwiftUI/AppKit app lifecycle.
    CFStringRef scheme   = CFSTR("sgdb");
    CFStringRef bundleID = CFSTR("com.steamgriddb.SGDBoop");
    LSSetDefaultHandlerForURLScheme(scheme, bundleID);
#pragma clang diagnostic pop
}

// ---------------------------------------------------------------------------
// ShowMessageBox — simple modal alert
// ---------------------------------------------------------------------------
int ShowMessageBox(const char* title, const char* message)
{
    NSApplication* app = [NSApplication sharedApplication];
    [app setActivationPolicy:NSApplicationActivationPolicyAccessory];
    [app activateIgnoringOtherApps:YES];

    NSAlert* alert        = [[NSAlert alloc] init];
    alert.messageText     = [NSString stringWithUTF8String:title   ? title   : ""];
    alert.informativeText = [NSString stringWithUTF8String:message ? message : ""];
    [alert addButtonWithTitle:@"OK"];
    alert.alertStyle = NSAlertStyleInformational;

    [alert runModal];

    return 0;
}

// ---------------------------------------------------------------------------
// Helpers for SelectionDialog
// ---------------------------------------------------------------------------

// Table data source / delegate backed by a plain C array of UTF-8 strings
@interface SGDBListDataSource : NSObject <NSTableViewDataSource, NSTableViewDelegate>
@property (nonatomic, assign) const char** items;
@property (nonatomic, assign) int          count;
@end

@implementation SGDBListDataSource
- (NSInteger)numberOfRowsInTableView:(NSTableView*)tv { (void)tv; return _count; }
- (NSView*)tableView:(NSTableView*)tv viewForTableColumn:(NSTableColumn*)col row:(NSInteger)row
{
    (void)col;
    NSTextField* cell = [tv makeViewWithIdentifier:@"cell" owner:self];
    if (!cell) {
        cell = [NSTextField labelWithString:@""];
        cell.identifier = @"cell";
    }
    cell.stringValue = [NSString stringWithUTF8String:(_items && row < _count) ? _items[row] : ""];
    return cell;
}
@end

// Thin NSObject that holds a block and fires it on -fire:
// Used to wire button actions without requiring a controller object.
@interface SGDBTrampoline : NSObject
@property (nonatomic, copy) dispatch_block_t block;
- (void)fire:(id)sender;
@end

@implementation SGDBTrampoline
- (void)fire:(id)sender { (void)sender; if (_block) _block(); }
@end

// Window delegate: closing the window should cancel the dialog
@interface SGDBDialogWindowDelegate : NSObject <NSWindowDelegate>
@property (nonatomic, copy) dispatch_block_t cancelBlock;
@end

@implementation SGDBDialogWindowDelegate
- (void)windowWillClose:(NSNotification*)n
{
    (void)n;
    if (_cancelBlock) _cancelBlock();
}
@end

// ---------------------------------------------------------------------------
// SelectionDialog — two-tab window (Non-Steam / GoldSrc+Source Mods)
// Returns the selected index (offset into the combined list), or -1 on cancel.
// ---------------------------------------------------------------------------
int SelectionDialog(const char* title,
                    int count,     const char** list,
                    int modsCount, const char** modsList,
                    int selection)
{
    NSApplication* app = [NSApplication sharedApplication];
    [app setActivationPolicy:NSApplicationActivationPolicyAccessory];

    // Resolve which tab and row to pre-select
    int preTab = 0;
    int preRow = selection;
    if (selection >= count && count > 0) {
        preTab = 1;
        preRow = selection - count;
    }

    __block int result = -1;

    // ------------------------------------------------------------------
    // Window
    // ------------------------------------------------------------------
    NSRect frame = NSMakeRect(0, 0, 480, 400);
    NSWindow* win = [[NSWindow alloc]
        initWithContentRect:frame
                  styleMask:(NSWindowStyleMaskTitled |
                             NSWindowStyleMaskClosable |
                             NSWindowStyleMaskResizable)
                    backing:NSBackingStoreBuffered
                      defer:NO];
    win.title = [NSString stringWithUTF8String:title ? title : "SGDBoop"];
    [win center];
    win.minSize = NSMakeSize(320, 260);

    // ------------------------------------------------------------------
    // Stop helper (wake the run loop and set result)
    // ------------------------------------------------------------------
    void (^stopLoop)(int r) = ^(int r) {
        result = r;
        [NSApp stop:nil];
        // Post a harmless event so -[NSApp stop:] actually unblocks the loop
        NSEvent* dummy = [NSEvent otherEventWithType:NSEventTypeApplicationDefined
                                            location:NSZeroPoint
                                       modifierFlags:0
                                           timestamp:0
                                        windowNumber:0
                                             context:nil
                                             subtype:0
                                               data1:0
                                               data2:0];
        [NSApp postEvent:dummy atStart:YES];
    };

    // ------------------------------------------------------------------
    // Tab view + two scrollable tables
    // ------------------------------------------------------------------
    NSTabView* tabView = [[NSTabView alloc] init];

    NSTableView* __block tableNS   = nil;
    NSTableView* __block tableMods = nil;

    // Keep data sources alive alongside the window
    NSMutableArray* retainedObjects = [NSMutableArray array];

    for (int tab = 0; tab < 2; tab++) {
        const char** tabItems = (tab == 0) ? list     : modsList;
        int          tabCount = (tab == 0) ? count    : modsCount;

        SGDBListDataSource* ds = [[SGDBListDataSource alloc] init];
        ds.items = tabItems;
        ds.count = tabCount;
        [retainedObjects addObject:ds];

        NSTableView* tv = [[NSTableView alloc] init];
        NSTableColumn* col = [[NSTableColumn alloc] initWithIdentifier:@"name"];
        col.width = 420;
        col.resizingMask = NSTableColumnAutoresizingMask;
        [tv addTableColumn:col];
        tv.headerView              = nil;
        tv.dataSource              = ds;
        tv.delegate                = ds;
        tv.allowsEmptySelection    = YES;
        tv.allowsMultipleSelection = NO;
        tv.usesAlternatingRowBackgroundColors = YES;

        int rowToSelect = (tab == preTab) ? preRow : -1;
        if (rowToSelect >= 0 && rowToSelect < tabCount) {
            [tv selectRowIndexes:[NSIndexSet indexSetWithIndex:(NSUInteger)rowToSelect]
            byExtendingSelection:NO];
        }

        NSScrollView* sv = [[NSScrollView alloc] init];
        sv.documentView        = tv;
        sv.hasVerticalScroller = YES;
        sv.autohidesScrollers  = YES;
        sv.borderType          = NSBezelBorder;

        NSTabViewItem* item = [[NSTabViewItem alloc] init];
        item.label = (tab == 0) ? @"Non-Steam" : @"GoldSrc/Source Mods";
        item.view  = sv;
        [tabView addTabViewItem:item];

        if (tab == 0) tableNS   = tv;
        else          tableMods = tv;
    }

    [tabView selectTabViewItemAtIndex:preTab];

    // ------------------------------------------------------------------
    // Buttons
    // ------------------------------------------------------------------
    NSButton* okBtn     = [NSButton buttonWithTitle:@"OK"
                                             target:nil action:nil];
    NSButton* cancelBtn = [NSButton buttonWithTitle:@"Cancel"
                                             target:nil action:nil];
    okBtn.keyEquivalent     = @"\r";
    cancelBtn.keyEquivalent = @"\033";
    okBtn.bezelStyle     = NSBezelStyleRounded;
    cancelBtn.bezelStyle = NSBezelStyleRounded;

    SGDBTrampoline* okT = [[SGDBTrampoline alloc] init];
    okT.block = ^{
        NSInteger activeTabIdx = [tabView indexOfTabViewItem:[tabView selectedTabViewItem]];
        NSTableView* activeTv  = (activeTabIdx == 0) ? tableNS : tableMods;
        NSInteger row = [activeTv selectedRow];
        if (row >= 0) {
            stopLoop((int)row + (activeTabIdx == 0 ? 0 : count));
        }
        // If nothing selected, don't dismiss — user must pick something
    };

    SGDBTrampoline* cancelT = [[SGDBTrampoline alloc] init];
    cancelT.block = ^{ stopLoop(-1); };

    okBtn.target     = okT;
    okBtn.action     = @selector(fire:);
    cancelBtn.target = cancelT;
    cancelBtn.action = @selector(fire:);

    [retainedObjects addObject:okT];
    [retainedObjects addObject:cancelT];

    // Window close → cancel
    SGDBDialogWindowDelegate* winDel = [[SGDBDialogWindowDelegate alloc] init];
    winDel.cancelBlock = ^{ stopLoop(-1); };
    win.delegate = winDel;
    [retainedObjects addObject:winDel];

    // ------------------------------------------------------------------
    // Layout (manual frames — avoids Auto Layout complexity with NSTabView)
    // ------------------------------------------------------------------
    CGFloat margin = 12.0;
    CGFloat btnH   = 30.0;
    CGFloat btnW   = 90.0;

    NSView* content = win.contentView;
    CGFloat W = content.bounds.size.width;
    CGFloat H = content.bounds.size.height;

    CGFloat tabH = H - margin * 3 - btnH;
    tabView.frame = NSMakeRect(margin, margin * 2 + btnH, W - margin * 2, tabH);
    tabView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;

    okBtn.frame     = NSMakeRect(W - margin - btnW,           margin, btnW, btnH);
    cancelBtn.frame = NSMakeRect(W - margin - btnW * 2 - 8.0, margin, btnW, btnH);
    okBtn.autoresizingMask     = NSViewMinXMargin | NSViewMaxYMargin;
    cancelBtn.autoresizingMask = NSViewMinXMargin | NSViewMaxYMargin;

    [content addSubview:tabView];
    [content addSubview:okBtn];
    [content addSubview:cancelBtn];

    // ------------------------------------------------------------------
    // Show and run a nested event loop
    // ------------------------------------------------------------------
    [win makeKeyAndOrderFront:nil];
    [app activateIgnoringOtherApps:YES];

    // Scroll pre-selected row into view after the window is displayed
    if (preRow >= 0) {
        NSTableView* activeTv = (preTab == 0) ? tableNS : tableMods;
        [activeTv scrollRowToVisible:preRow];
    }

    [app run];

    win.delegate = nil;
    [win close];
    (void)retainedObjects; // keep alive until here
    return result;
}

// No-ops: Win32-internal helpers never called from sgdboop.c on non-Windows
void PopulateListBox(int tabIndex)                       { (void)tabIndex; }
void PopulateListBoxWithSelection(int tabIndex, int sel) { (void)tabIndex; (void)sel; }
