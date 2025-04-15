#include "gui-helper.h"
#include <gtk/gtk.h>

int ShowMessageBox(const char* title, const char* message)
{
	gtk_init(NULL, NULL);

	GtkWidget* dialog = gtk_message_dialog_new(
		NULL,
		0,
		GTK_MESSAGE_ERROR,
		GTK_BUTTONS_OK,
		"%s",
		message
	);
	gtk_window_set_title(GTK_WINDOW(dialog), title);

	g_signal_connect_swapped (
		dialog,
		"response",
		G_CALLBACK (gtk_widget_destroy),
		dialog
	);

	return gtk_dialog_run(GTK_DIALOG(dialog));
}

enum {
	COLUMN_ITEM = 0,
	COLUMN_INDEX = 1,
};

typedef struct {
	GtkWidget* treeview;
	int selection;
	int selected_index;
} TabData;

typedef struct {
	TabData* tab1;
	TabData* tab2;
} TabsContext;

static void button_callback(GtkWidget* widget, gpointer data)
{
	GtkWidget* treeview = (GtkWidget*)data;

	GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));

	GtkTreeModel* model;
	GtkTreeIter iter;
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		int* selected_index = g_object_get_data(G_OBJECT(treeview), "selected_index");
		gtk_tree_model_get(model, &iter, COLUMN_INDEX, selected_index, -1);
		gtk_main_quit();
	}
}

static void on_tab_switch(GtkNotebook* notebook, GtkWidget* page, guint page_num, gpointer user_data)
{
	TabsContext* ctx = (TabsContext*)user_data;
	GtkButton* btn = GTK_BUTTON(g_object_get_data(G_OBJECT(notebook), "ok_button"));

	if (page_num == 0)
		g_object_set_data(G_OBJECT(btn), "tabdata", ctx->tab1);
	else
		g_object_set_data(G_OBJECT(btn), "tabdata", ctx->tab2);
}


static void on_ok_button_clicked(GtkButton* button, gpointer user_data)
{
	TabData* data = (TabData*)g_object_get_data(G_OBJECT(button), "tabdata");
	button_callback(GTK_WIDGET(button), data);
}

GtkWidget* create_treeview(const char** items, int count, int initial_selection, int* selected_index)
{
	GtkListStore* store = gtk_list_store_new(
		2, // 2 tabs
		G_TYPE_STRING,
		G_TYPE_INT
	);
	GtkTreeIter iter;
	for (int i = 0; i < count; ++i) {
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, COLUMN_ITEM, items[i], COLUMN_INDEX, i, -1);
	}

	GtkWidget* treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeview), FALSE);

	GtkTreeSelection* treeselection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
	gtk_tree_selection_set_mode(treeselection, GTK_SELECTION_SINGLE);

	if (initial_selection >= 0 && initial_selection < count) {
		GtkTreePath* path = gtk_tree_path_new_from_indices(initial_selection, -1);
		gtk_tree_selection_select_path(treeselection, path);
		gtk_tree_path_free(path);
	}

	GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview), -1, "", renderer, "text", COLUMN_ITEM, NULL);

	return treeview;
}

int SelectionDialog(const char* title, int count, const char** list, int modsCount, const char** modsList, int selection)
{
	gtk_init(NULL, NULL);
	int selected_index = -1;

	// Stylesheet
	const char* stylesheet = "#box { padding: 10px; }";
	GtkCssProvider* provider = gtk_css_provider_new();
	gtk_css_provider_load_from_data(provider, stylesheet, -1, NULL);
	GdkDisplay* display = gdk_display_get_default();
	GdkScreen* screen = gdk_display_get_default_screen(display);
	gtk_style_context_add_provider_for_screen(screen, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);

	// Setup main window
	GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), title);
	gtk_window_set_default_size(GTK_WINDOW(window), 454, 388);
	g_signal_connect(window, "destroy", gtk_main_quit, NULL);

	GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
	gtk_widget_set_name(box, "box");
	gtk_container_add(GTK_CONTAINER(window), box);

	// Tabs via GtkNotebook
	GtkWidget* notebook = gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX(box), notebook, TRUE, TRUE, 0);

	// Non-Steam apps tab
	TabData* nonsteam_data = g_new0(TabData, 1);
	nonsteam_data->treeview = create_treeview(list, count, selection, &nonsteam_data->selected_index);

	GtkWidget* scroll1 = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(scroll1), nonsteam_data->treeview);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), scroll1, gtk_label_new("Non-Steam Apps"));

	// Mods tab
	TabData* mods_data = g_new0(TabData, 1);
	mods_data->treeview = create_treeview(modsList, modsCount, 0, &mods_data->selected_index);

	GtkWidget* scroll2 = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(scroll2), mods_data->treeview);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), scroll2, gtk_label_new("Mods"));

	TabsContext* context = g_malloc(sizeof(TabsContext));
	context->tab1 = nonsteam_data;
	context->tab2 = mods_data;

	// Button contianer
	GtkWidget* buttons = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_box_pack_end(GTK_BOX(box), buttons, FALSE, FALSE, 0);
	gtk_widget_set_halign(buttons, GTK_ALIGN_END);
	gtk_widget_set_valign(buttons, GTK_ALIGN_START);

	// COnfirm button
	GtkWidget* ok_button = gtk_button_new_with_label("OK");
	gtk_widget_set_size_request(ok_button, 75, 40);
	g_signal_connect(ok_button, "clicked", G_CALLBACK(button_callback), nonsteam_data) // nonsteam_data is changed below
	gtk_container_add(GTK_CONTAINER(buttons), ok_button);
	g_object_set_data(G_OBJECT(notebook), "ok_button", ok_button);

	// Cancel button
	GtkWidget* cancel_button = gtk_button_new_with_label("Cancel");
	gtk_widget_set_size_request(cancel_button, 75, 40);
	g_signal_connect(cancel_button, "clicked", gtk_main_quit, NULL);
	gtk_container_add(GTK_CONTAINER(buttons), cancel_button);

	// Switch callback to update which tab is active
	g_signal_connect(notebook, "switch-page", G_CALLBACK(on_tab_switch), context);

	// initial tab data
	g_object_set_data(G_OBJECT(ok_button), "tabdata", nonsteam_data);

	// Update button callback to extract correct data
	g_signal_connect(ok_button, "clicked", G_CALLBACK(on_ok_button_clicked), NULL);

	gtk_widget_show_all(window);
	gtk_main();

	// Clean up and return
	int final_selection = ((TabData*)g_object_get_data(G_OBJECT(ok_button), "tabdata"))->selected_index;
	g_free(nonsteam_data);
	g_free(mods_data);
	return final_selection;
}
